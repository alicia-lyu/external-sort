#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include "InMemoryRenderer.h"
#include "GracefulRenderer.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, RowSize const size, RowCount const count, bool removeDuplicates) : 
	_input (input), _size (size), _count (count),
	_recordCountPerRun (getRecordCountPerRun(size, true)),
	_removeDuplicates (removeDuplicates)
{
	TRACE (false);
	traceprintf ("SortPlan: memory space %d, record per run %d\n", MEMORY_SIZE, _recordCountPerRun);
	if (_removeDuplicates)
		traceprintf ("SortPlan: remove duplicates using in-sort method\n");
} // SortPlan::SortPlan

SortPlan::~SortPlan ()
{
	TRACE (false);
	delete _input;
} // SortPlan::~SortPlan

Iterator * SortPlan::init () const
{
	TRACE (false);
	return new SortIterator (this);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()), _consumed (0), _produced (0)
{
	TRACE (false);
	if (_plan->_count <= _plan->_recordCountPerRun) {
		_renderer = _formInMemoryRenderer();
	} else {
		_renderer = _externalSort();
	}
	traceprintf ("consumed %lu rows\n",
			(unsigned long) (_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);
	delete _input;
	delete _renderer;
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // SortIterator::~SortIterator

byte * SortIterator::next ()
{
	byte * row = _renderer->next();
	// renderer.next produces rows in the sorted order
	// All preliminary work (incl. creating initial runs, first n-1 level merge)
	// is done before the first next() call
	if (row == nullptr) return nullptr;
	++ _produced;
	// traceprintf ("#%llu produced %s\n", _produced, rowToHexString(row, _plan->_size).c_str());
	return row;
} // SortIterator::next


SortedRecordRenderer * SortIterator::_formInMemoryRenderer (RowCount base, u_int16_t runNumber, u_int32_t memory_limit, bool materialize)
{
	vector<byte *> rows;
	while ((_consumed - base) * _plan->_size < memory_limit) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		rows.push_back(received);
		++ _consumed;
	}

	#if defined(VERBOSEL2)
	traceprintf ("Forming in-memory renderer with %lu rows\n", rows.size());
	#endif

	// break rows by cache size
	// _plan->_size: size of each row
	// CACHE_SIZE: size of cache
	int rowsPerCache = CACHE_SIZE / _plan->_size;
	RowCount numCaches = (rows.size() + rowsPerCache - 1) / rowsPerCache;

	#if defined(VERBOSEL2)
	traceprintf ("Cache size %d, row size %hu, rows per cache %d, number of caches %llu\n",
		CACHE_SIZE, _plan->_size, rowsPerCache, numCaches);
	#endif

	vector<TournamentTree *> cacheTrees;
	for (int i = 0; i < numCaches; i++) {
		// start is inclusive, end is exclusive
		int start = i * rowsPerCache;
		int end = std::min((i + 1) * rowsPerCache, (int) rows.size());
		vector<byte *> cacheRows(rows.begin() + start, rows.begin() + end);

		#ifdef VERBOSEL2
		traceprintf ("In-memory renderer forming cache tree %d with rows #%d-#%d\n",
			i, start, end - 1);
		#endif

		TournamentTree * tree = new TournamentTree(cacheRows, _plan->_size);
		cacheTrees.push_back(tree);
	}

	SortedRecordRenderer * renderer = new CacheOptimizedRenderer(_plan->_size, cacheTrees, runNumber, _plan->_removeDuplicates, materialize);
	return renderer;
}

vector<string> SortIterator::_createInitialRuns () // metrics
{
	vector<string> runNames;
	while (_consumed < _plan->_count) {
		SortedRecordRenderer * renderer = _formInMemoryRenderer(_consumed, runNames.size());
		string runName = renderer->run();
		delete renderer; // Only after deleting the renderer, the run file is flushed and closed
		runNames.push_back(runName);
	}
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Created %lu initial runs\n", runNames.size());
	#endif
	return runNames;
}

SortedRecordRenderer * SortIterator::gracefulDegradation ()
{
	// Determine the size of the initial in-memory run (the only intermediate spill)
	u_int64_t gracefulInMemoryRunSize = MEMORY_SIZE - SSD_PAGE_SIZE * 3; // One page for output, one page for external renderer run page, one page for external renderer read-ahead
	u_int64_t initialInMemoryRunSize = _plan->_size * _plan->_count - gracefulInMemoryRunSize;
	Assert (initialInMemoryRunSize <= MEMORY_SIZE - SSD_PAGE_SIZE, __FILE__, __LINE__);
	Assert (_consumed == 0, __FILE__, __LINE__);

	// Create the initial in-memory run and materialize to disk
	string initialRunFileName = _formInMemoryRenderer(_consumed, 0, initialInMemoryRunSize)->run();

	// Create the remainder in-memory renderer
	SortedRecordRenderer * inMemoryRenderer = _formInMemoryRenderer(_consumed, 1, gracefulInMemoryRunSize, false);

	// Create the external run
	ExternalRun::READ_AHEAD_SIZE = SSD_PAGE_SIZE;
    ExternalRun::READ_AHEAD_THRESHOLD = std::max(0.5, ((double) SSD_PAGE_SIZE) / MEMORY_SIZE);
	ExternalRun * externalRun = new ExternalRun(initialRunFileName, _plan->_size);

	// GRACEFUL DEGRADATION
	SortedRecordRenderer * gracefulRenderer = new GracefulRenderer(_plan->_size, inMemoryRenderer, externalRun, _plan->_removeDuplicates);

	return gracefulRenderer;
}

SortedRecordRenderer * SortIterator::_externalSort ()
{
	TRACE (false);

	if (_plan->_count * _plan->_size <= MEMORY_SIZE * GRACEFUL_DEGRADATION_THRESHOLD) {
		return gracefulDegradation();
	}

	vector<string> runNames = _createInitialRuns();
	u_int8_t pass = 0;
	SortedRecordRenderer * renderer = nullptr;
	// Multi-pass merge
	while (runNames.size() > 1) { // Need another pass to merge the runs
		++ pass;
		#if defined(VERBOSEL1) || defined(VERBOSEL2)
		traceprintf ("Pass %d: %lu runs in total\n", pass, runNames.size());
		#endif

		u_int16_t mergedRunCount = 0;
		vector<string> mergedRunNames;
		u_int16_t rendererNum = 0;

		while (mergedRunCount < runNames.size()) { // has more runs to merge
			// one renderer
			auto [mergedRunCountNew, readAheadSize, allMemoryNeeded] = assignRuns(runNames, mergedRunCount);
			u_int16_t mergedRunCountSoFar = mergedRunCount;
			mergedRunCount = mergedRunCountNew;

			if (allMemoryNeeded <= MEMORY_SIZE * GRACEFUL_DEGRADATION_THRESHOLD && allMemoryNeeded > MEMORY_SIZE) {
				#if defined(VERBOSEL1) || defined(VERBOSEL2)
				traceprintf("*** Graceful merge in pass %d: merged run count new %d, all memory needed %llu\n", pass, mergedRunCount, allMemoryNeeded);
				#endif
				std::vector<string> restOfRuns = std::vector<string>(runNames.begin() + mergedRunCountSoFar, runNames.end());
				renderer = gracefulMerge(restOfRuns, pass, rendererNum);
				mergedRunCount = runNames.size(); // All are merged gracefully
			} else {
				#if defined(VERBOSEL1) || defined(VERBOSEL2)
				traceprintf ("Pass %d renderer %d: Merging runs from %d to %d out of %zu\n", pass, rendererNum, mergedRunCountSoFar, mergedRunCount - 1, runNames.size() - 1);
				#endif
				renderer = new ExternalRenderer(_plan->_size, 
				vector<string>(runNames.begin() + mergedRunCountSoFar, runNames.begin() + mergedRunCount), 
				readAheadSize, pass, rendererNum, _plan->_removeDuplicates);
			}

			if (rendererNum == 0 && mergedRunCount == runNames.size()) // The last renderer in the last pass
			{
				#if defined(VERBOSEL1) || defined(VERBOSEL2)
				traceprintf("*** Multi-level merge stopped in pass %d\n", pass);
				#endif
				return renderer;
			} else { // Need another pass for merged runs
				mergedRunNames.push_back(renderer->run());
				rendererNum++;
				delete renderer;
			}
		}

		runNames = mergedRunNames;
	}

	throw std::runtime_error("External sort not returning renderer.");
}

// Return: mergedRunCount, readAheadSize, neededMemorySize
// mergedRunCount: the number of runs that can be merged in this pass
// readAheadSize: the size of read-ahead buffers needed in this pass
// neededMemorySize: if merge all runs in this pass, the memory needed
tuple<u_int16_t, u_int64_t, u_int64_t> SortIterator::assignRuns(vector<string>& runNames, u_int16_t mergedRunCount) 
{
	u_int64_t readAheadSize= profileReadAheadSize(runNames, mergedRunCount);
	u_int64_t outputPageSize = profileOutputPageSize(runNames, mergedRunCount);

	// Calculate all memory consumption till the end of runNames
	u_int64_t allMemoryConsumption = readAheadSize + outputPageSize;
	for (int i = mergedRunCount; i < runNames.size(); i++) {
		auto deviceType = getLargestDeviceType(runNames.at(i));
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		allMemoryConsumption += pageSize;
	}

	// INPUT BUFFERS
	u_int64_t memoryConsumption = readAheadSize + outputPageSize;
	while (mergedRunCount < runNames.size()) { // max. 120 G / 98 M = 2^11

		string &runName = runNames.at(mergedRunCount);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;

		if (memoryConsumption + pageSize > MEMORY_SIZE) break;

		memoryConsumption += pageSize;
		mergedRunCount++;
	}

	Assert (memoryConsumption <= MEMORY_SIZE, __FILE__, __LINE__);
	readAheadSize += MEMORY_SIZE - memoryConsumption; // Use the remaining memory for more read-ahead buffers if any

	return std::make_tuple(mergedRunCount, readAheadSize, allMemoryConsumption);
}

u_int64_t SortIterator::profileReadAheadSize (vector<string>& runNames, u_int16_t mergedRunCount)
{
	TRACE (false);
	u_int64_t memoryConsumption = 0;
	u_int16_t ssdRunCount = 0;
	u_int16_t hddRunCount = 0;
	for (int i = mergedRunCount; i < runNames.size(); i++) {
		auto deviceType = getLargestDeviceType(runNames.at(i));
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		memoryConsumption += pageSize;
		if (memoryConsumption > MEMORY_SIZE) break;
		else {
			if (deviceType == 0) ssdRunCount++;
			else hddRunCount++;
		}
	}
	double hddRunRatio = (double) hddRunCount / (hddRunCount + ssdRunCount);
	int hddBufferCount;
	if (hddRunRatio > 0.67) hddBufferCount = 2;
	else if (hddRunCount > 0.33) hddBufferCount = 1;
	else return hddBufferCount = 0;
	u_int64_t readAheadSize = hddBufferCount * HDD_PAGE_SIZE + 
		(READ_AHEAD_BUFFERS_MIN - hddBufferCount) * SSD_PAGE_SIZE;
	return readAheadSize;
}

u_int64_t SortIterator::profileOutputPageSize (vector<string>& runNames, u_int16_t mergedRunCount)
{
	TRACE (false);
	// A conservative estimate of the output page size
	// The profiled outputFileSize is the largest possible
	u_int64_t memoryConsumption = 0;
	u_int64_t outputFileSize = 0;
	while (mergedRunCount < runNames.size()) {

		string &runName = runNames.at(mergedRunCount);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;

		if (memoryConsumption + pageSize > MEMORY_SIZE) break;

		memoryConsumption += pageSize;
		mergedRunCount++;
		outputFileSize += std::filesystem::file_size(runName);
	}
	int deviceType = Metrics::getAvailableStorage(outputFileSize);
	// Allocate output buffer conservatively: max page sizes of all storage devices needed
	// But will try to write into SSD first if possible
	auto outputPageSize = Metrics::getParams(deviceType).pageSize;
	return outputPageSize;
}

SortedRecordRenderer * SortIterator::gracefulMerge (vector<string>& runNames, int basePass, int rendererNum)
{
	TRACE (false);

	// Optimization problem:
	// Memory consumption of the graceful renderer <= MEMORY_SIZE
	// Minimize the size of the initial run so that 
	// 1) Extra spill is minimized
	// 2) Extra spill is into SSD if possible (to avoid HDD I/O)

	auto gracefulReadAheadSize = profileReadAheadSize(runNames, 0); 
	// Only the first few runs whose pages can fit into memory are profiled --- for graceful renderer
	// It is assumed that the initial renderer would have plenty of available memory for reading ahead
	// Because we are minimizing the size of the initial run

	int n = runNames.size();

	u_int64_t inputMemoryForAllRuns = 0; // Input buffer size to contain pages from all runs
	u_int64_t allRunSize = 0; // Total size of all runs
	for (int i = 0; i < n; i++) {
		auto deviceType = getLargestDeviceType(runNames.at(i));
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		inputMemoryForAllRuns += pageSize;
		allRunSize += std::filesystem::file_size(runNames.at(i));
	}

	int const gracefulOutputDevice = Metrics::getAvailableStorage(allRunSize);
	
	u_int64_t initialRunSize = 0;
	u_int64_t initialPageSize;
	u_int64_t initialInputMemory = 0; // output page size for initialRenderer, input page size of initialRun for gracefulRenderer
	u_int64_t const gracefulOutputPage = Metrics::getParams(gracefulOutputDevice).pageSize;
	u_int64_t const gracefulMemoryFixed = gracefulOutputPage + gracefulReadAheadSize;

	// Find the smallest initial run size
	int i;
	for (i = 1; i <= n; i++) {
		// the last i runs are merged in the initial renderer
		string runName = runNames.at(n - i);
		initialRunSize += std::filesystem::file_size(runName);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		initialInputMemory += pageSize;

		auto initialRunDevice = Metrics::getAvailableStorage(initialRunSize);
		initialPageSize = Metrics::getParams(initialRunDevice).pageSize; 
		if (gracefulMemoryFixed + inputMemoryForAllRuns - initialInputMemory + initialPageSize <= MEMORY_SIZE) // Memory consumption of the graceful renderer
		{
			break;
		}
	}

	// Create the initial run
	traceprintf ("Creating initial run with %d runs. Run size %llu, page size %llu, input memory %llu\n", i, initialRunSize, initialPageSize, initialInputMemory);

	u_int64_t initialReadAheadSize = MEMORY_SIZE - initialInputMemory - initialPageSize;
	Assert (initialReadAheadSize >= 0, __FILE__, __LINE__);

	ExternalRenderer * initialRenderer = new ExternalRenderer(_plan->_size, 
		vector<string>(runNames.end() - i, runNames.end()),
		initialReadAheadSize, basePass, rendererNum + 1, _plan->_removeDuplicates
	); // rendererNum + 1: This run will not be pushed back to mergedRunNames in _externalSort
	string initialRunFileName = initialRenderer->run();

	// Create the graceful renderer for the rest of the runs + initial run
	traceprintf ("Creating graceful renderer with %d runs, to device %d\n", n - i, gracefulOutputDevice);
	vector<string> restOfRuns = std::vector<string>(runNames.begin(), runNames.end() - i);
	restOfRuns.push_back(initialRunFileName);
	SortedRecordRenderer * gracefulRenderer = new ExternalRenderer(_plan->_size, restOfRuns, gracefulReadAheadSize, basePass, rendererNum, _plan->_removeDuplicates);

	return gracefulRenderer;
	
}