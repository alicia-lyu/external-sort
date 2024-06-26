#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include "InMemoryRenderer.h"
#include "GracefulRenderer.h"
#include "ExternalSorter.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, RowSize const size, RowCount const count, bool removeDuplicates, std::function<void ()> forceFlushBufferHandler) : 
	_input (input), _size (size), _count (count),
	_removeDuplicates (removeDuplicates)
{
	TRACE (false);
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
	auto recordCountPerRun = getRecordCountPerRun(_plan->_size);
	#ifdef PRODUCTION
	string output = "Memory space: ";
	output += to_string(MEMORY_SIZE) + " bytes";
	// if memory size is larger than 1KB, also print in formatted size
	if (MEMORY_SIZE > 1000) {
		output += " (";
		output += Trace::FormatSize(MEMORY_SIZE);
		output += ")";
	}
	output += ", record count per run: " + to_string(recordCountPerRun);

	Trace::PrintTrace(OP_STATE, INIT_SORT, output);
	if (_plan->_removeDuplicates)
		Trace::PrintTrace(OP_STATE, INIT_SORT, "Remove duplicates using in-sort method");
	#endif
	if (_plan->_count <= recordCountPerRun) {
		_renderer = _formInMemoryRenderer();
	} else {
		_renderer = _externalSort();
	}
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("consumed %lu rows\n",
			(unsigned long) (_consumed));
	#endif
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (false);
	delete _input;
	delete _renderer;

	#ifdef PRODUCTION
	Trace::PrintTrace(OP_RESULT, SORT_RESULT, "SortPlan produced " + to_string(_produced) + " of " + to_string(_consumed) + " rows");
	#endif

	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
	#endif
} // SortIterator::~SortIterator

byte * SortIterator::next ()
{
	byte * row = _renderer->next();
	// renderer.next produces rows in the sorted order
	// All preliminary work (incl. creating initial runs, first n-1 level merge)
	// is done before the first next() call
	if (row == nullptr) return nullptr;
	++ _produced;
	if (_produced % 10000 == 0) {
		#if defined(VERBOSEL1)
		traceprintf ("#%llu produced %s\n", _produced, rowToString(row, _plan->_size).c_str());
		#endif
	}
	return row;
} // SortIterator::next


SortedRecordRenderer * SortIterator::_formInMemoryRenderer (RowCount base, u_int16_t runNumber, u_int32_t memory_limit, bool materialize)
{
	bool forceFlushBuffer = false;
	auto scanBufferSize = getRecordCountPerRun(_plan->_size) * _plan->_size;
	if (memory_limit == 0) {
		memory_limit = scanBufferSize; 
		// Should be the same as the last returned value of getRecordCountPerRun called in ScanIterator
	} else {
		Assert(memory_limit <= scanBufferSize, __FILE__, __LINE__);
		forceFlushBuffer = true;
		// Not the same as the last returned value of getRecordCountPerRun called in ScanIterator
		// Forcing the buffer to flush
	}
	vector<byte *> rows;
	while ((_consumed - base + 1) * _plan->_size <= memory_limit) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		auto recordCountPerRun = getRecordCountPerRun(_plan->_size);
		if (_consumed - base >= recordCountPerRun) {
			throw std::runtime_error("In-memory renderer exceeds the record count per run " + std::to_string(_consumed - base) + "/" + std::to_string(recordCountPerRun) + " at " + std::to_string(_consumed) + " limit " + std::to_string(memory_limit / _plan->_size));
		}
		rows.push_back(received);
		++ _consumed;
	}

	if (forceFlushBuffer) {
		bool result = _input->forceFlushBuffer();
		Assert(result, __FILE__, __LINE__);
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
	for (RowCount i = 0; i < numCaches; i++) {
		// start is inclusive, end is exclusive
		int start = i * rowsPerCache;
		int end = std::min((i + 1) * rowsPerCache, rows.size());
		#if defined(VERBOSEL2)
		traceprintf ("Cache %d: start %d, end %d\n", i, start, end);
		#endif
		vector<byte *> cacheRows(rows.begin() + start, rows.begin() + end);

		TournamentTree * tree = new TournamentTree(cacheRows, _plan->_size);
		cacheTrees.push_back(tree);
	}

	SortedRecordRenderer * renderer = new CacheOptimizedRenderer(_plan->_size, cacheTrees, runNumber, _plan->_removeDuplicates, materialize);
	// TournamentTree * tree = new TournamentTree(rows, _plan->_size);
	// SortedRecordRenderer * renderer = new NaiveRenderer(_plan->_size, tree, runNumber, _plan->_removeDuplicates, materialize);
	return renderer;
}

vector<string> SortIterator::_createInitialRuns () // metrics
{
	vector<string> runNames;
	int runNum = 0;
	while (_consumed < _plan->_count) {
		SortedRecordRenderer * renderer = _formInMemoryRenderer(_consumed, runNames.size());
		string runName = renderer->run();
		delete renderer; // Only after deleting the renderer, the run file is flushed and closed
		runNames.push_back(runName);

		++runNum;
		Trace::PrintStdout("Initial run #%d: %d / %d (%.2lf%%)\n", runNum, _consumed, _plan->_count, 100.0 * _consumed / _plan->_count);
	}
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Created %lu initial runs\n", runNames.size());
	#endif
	return runNames;
}

SortedRecordRenderer * SortIterator::gracefulDegradation ()
{
	Trace::PrintStdout("GRACEFUL DEGRADATION\n");
	// Determine the size of the initial in-memory run (the only intermediate spill)
	int storageProfiled = Metrics::getAvailableStorage(MEMORY_SIZE * GRACEFUL_DEGRADATION_THRESHOLD); // Expected to be SSD, as the data is barely larger than memory
	u_int64_t gracefulInMemoryRunSize = (MEMORY_SIZE - Metrics::getParams(storageProfiled).pageSize * 3) / _plan->_size * _plan->_size; // One page for output, one page for external renderer run page, one page for external renderer read-ahead
	u_int64_t initialInMemoryRunSize = _plan->_size * _plan->_count - gracefulInMemoryRunSize;
	Assert (initialInMemoryRunSize <= getRecordCountPerRun(_plan->_size) * _plan->_size, __FILE__, __LINE__);
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
	ExternalSorter * sorter = new ExternalSorter(runNames, _plan->_size, _plan->_removeDuplicates);
	return sorter->init();
}