#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include "InMemoryRenderer.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, RowSize const size, RowCount const count, bool removeDuplicates) : 
	_input (input), _size (size), _count (count),
	_recordCountPerRun (getRecordCountPerRun(size, true)),
	_removeDuplicates (removeDuplicates)
{
	traceprintf ("SortPlan: memory space %d, record per run %d\n", MEMORY_SIZE, _recordCountPerRun);
} // SortPlan::SortPlan

SortPlan::~SortPlan ()
{
	TRACE (false);
	delete _input;
} // SortPlan::~SortPlan

Iterator * SortPlan::init () const
{
	TRACE (true);
	return new SortIterator (this);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()), _consumed (0), _produced (0)
{
	TRACE (true);
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
	TRACE (true);
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


SortedRecordRenderer * SortIterator::_formInMemoryRenderer (RowCount base, u_int16_t runNumber)
{
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Forming in-memory renderer with %u rows\n", _plan->_recordCountPerRun);
	#endif

	vector<byte *> rows;
	while (_consumed - base < _plan->_recordCountPerRun) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		rows.push_back(received);
		++ _consumed;
	}

	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Formed in-memory renderer with %lu rows\n", rows.size());
	#endif

	// break rows by cache size
	// _plan->_size: size of each row
	// CACHE_SIZE: size of cache
	int rowsPerCache = CACHE_SIZE / _plan->_size;
	RowCount numCaches = (rows.size() + rowsPerCache - 1) / rowsPerCache;

	#if defined(VERBOSEL1) || defined(VERBOSEL2)
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

	SortedRecordRenderer * renderer = new CacheOptimizedRenderer(_plan->_size, cacheTrees, runNumber, _plan->_removeDuplicates);
	
	// NaiveRenderer: Not cache-optimized
	// TournamentTree * tree = new TournamentTree(rows, _plan->_size);
	// SortedRecordRenderer * renderer = new NaiveRenderer(_plan->_size, tree);
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

SortedRecordRenderer * SortIterator::_externalSort ()
{
	TRACE (true);
	vector<string> runNames = _createInitialRuns();
	u_int8_t pass = 0;
	// Multi-pass merge
	while (true) { // one pass
		++ pass;
		#if defined(VERBOSEL1) || defined(VERBOSEL2)
		traceprintf ("Pass %d: %lu runs in total\n", pass, runNames.size());
		#endif
		u_int16_t mergedRunCount = 0;
		vector<string> mergedRunNames;
		u_int16_t rendererNum = 0;
		while (mergedRunCount < runNames.size()) { // one renderer
			u_int16_t mergedRunCountSoFar = mergedRunCount;
			u_int64_t memoryConsumption = 0;
			// OUTPUT BUFFER
			int deviceType = Metrics::getAvailableStorage();
			auto outputPageSize = Metrics::getParams(deviceType).pageSize;
			memoryConsumption += outputPageSize;

			// READ-AHEAD BUFFERS
			auto hddReadAheadBuffers = profileReadAheadBuffers(runNames, mergedRunCount);
			u_int64_t readAheadSize = hddReadAheadBuffers * Metrics::getParams(1).pageSize + (READ_AHEAD_BUFFERS_MIN - hddReadAheadBuffers) * Metrics::getParams(0).pageSize;
			memoryConsumption += readAheadSize;

			// INPUT BUFFERS
			while (mergedRunCount < runNames.size()) { // max. 120 G / 98 M = 2^11
				int deviceType = parseDeviceType(runNames[mergedRunCount]);
				memoryConsumption += Metrics::getParams(deviceType).pageSize; // 1 input buffer for this run
				if (memoryConsumption > MEMORY_SIZE) {
					memoryConsumption -= Metrics::getParams(deviceType).pageSize;
					break;
				}
				mergedRunCount++;
			}
			readAheadSize += MEMORY_SIZE - memoryConsumption; // Use the remaining memory for more read-ahead buffers
			// MERGE RUNS
			ExternalRenderer * renderer = new ExternalRenderer(_plan->_size, 
				vector<string>(runNames.begin() + mergedRunCountSoFar, runNames.begin() + mergedRunCount), readAheadSize,
				pass, rendererNum);
			if (rendererNum == 0 && mergedRunCount == runNames.size()) {
				// The last renderer in the last pass
				return renderer;
			}
			mergedRunNames.push_back(renderer->run());
			delete renderer;
			rendererNum++;
		}
		runNames = mergedRunNames;
	}
}

u_int8_t SortIterator::profileReadAheadBuffers (vector<string>& runNames, u_int16_t mergedRunCount)
{
	TRACE (true);
	u_int64_t memoryConsumption = 0;
	u_int16_t ssdRunCount = 0;
	u_int16_t hddRunCount = 0;
	for (int i = mergedRunCount; i < runNames.size(); i++) {
		auto deviceType = parseDeviceType(runNames[i]);
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		memoryConsumption += pageSize;
		if (memoryConsumption > MEMORY_SIZE) break;
		else {
			if (deviceType == 0) ssdRunCount++;
			else hddRunCount++;
		}
	}
	double hddRunRatio = (double) hddRunCount / (hddRunCount + ssdRunCount);
	if (hddRunRatio > 2/3) return 2;
	else if (hddRunCount > 1/3) return 1;
	else return 0;
}