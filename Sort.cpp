#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, RowSize const size, RowCount const count) : 
	_input (input), _size (size), _count (count), _recordCountPerRun (getRecordCountPerRun(size, true))
{
	// TODO: introduce HDD --- preliminary thought: Just add predicates to indicate whether SSD is full.
	// Use appropriate metrics. No change in code logic except page size
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

	SortedRecordRenderer * renderer = new CacheOptimizedRenderer(_plan->_size, cacheTrees, runNumber);
	
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
		runNames.push_back(runName);
	}
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Created %lu initial runs\n", runNames.size());
	#endif
	return runNames;
}

SortedRecordRenderer * SortIterator::_externalSort ()
{
	vector<string> runNames = _createInitialRuns(); // runNames is modified in this function, as it is passed by reference
	u_int16_t inputBufferCount = MEMORY_SIZE / SSD_PAGE_SIZE - 1 - READ_AHEAD_BUFFERS_MIN; // TODO: Implement read-ahead buffers
	u_int8_t totalPasses = std::ceil(std::log(runNames.size()) / std::log(inputBufferCount));
	traceprintf ("Total passes: %d\n", totalPasses);
	SortedRecordRenderer * renderer = new ExternalRenderer(_plan->_size, runNames, totalPasses); // last pass: output
	return renderer;
}