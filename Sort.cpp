#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include "InMemoryRenderer.h"
#include "GracefulRenderer.h"
#include "ExternalSorter.h"
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
	ExternalSorter * sorter = new ExternalSorter(runNames, _plan->_size, _plan->_removeDuplicates);
	return sorter->init();
}

// Return: mergedRunCount, readAheadSize, neededMemorySize
// mergedRunCount: the number of runs that can be merged in this pass
// readAheadSize: the size of read-ahead buffers needed in this pass
// neededMemorySize: if merge all runs in this pass, the memory needed
