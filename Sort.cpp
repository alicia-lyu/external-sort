#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, RowSize const size, RowCount const count) : 
	_input (input), _size (size), _count (count), _recordCountPerRun (getRecordCountPerRun(size, true)) // TODO: introduce HDD
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


SortedRecordRenderer * SortIterator::_formInMemoryRenderer (RowCount base)
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

		#ifdef VERBOSEL2
		traceprintf ("#%llu consumed %s\n", _consumed, rowToString(received, _plan->_size).c_str());
		#endif
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

	SortedRecordRenderer * renderer = new CacheOptimizedRenderer(cacheTrees, _plan->_size);
	
	// NaiveRenderer: Not cache-optimized
	// TournamentTree * tree = new TournamentTree(rows, _plan->_size);
	// SortedRecordRenderer * renderer = new NaiveRenderer(tree);
	return renderer;
}

void SortIterator::_createInitialRuns (vector<string> &runNames)
{
	runNames.clear();
	Buffer * outputBuffer = new Buffer(_plan->_recordCountPerRun, _plan->_size);
	while (_consumed < _plan->_count) {
		outputBuffer->reset();
		string runName = string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass0") + SEPARATOR + string("run") + std::to_string(_consumed / _plan->_recordCountPerRun) + string(".bin");
		runNames.push_back(runName);
		SortedRecordRenderer * renderer = _formInMemoryRenderer(_consumed);
		int i;
		for (i = 0; i < _plan->_recordCountPerRun; i++) {
			byte * row = renderer->next();
			if (row == nullptr) break;
			if (outputBuffer->copy(row) == nullptr) {
				throw std::runtime_error("Output buffer overflows when creating initial run " + runName + ".\n");
			}
		}
		std::ofstream runFile(runName, std::ios::binary);
		int outputBufferSize = i * _plan->_size;
		runFile.write(reinterpret_cast<char *>(outputBuffer->data()), outputBufferSize);
		traceprintf ("Created run file %s with size %d\n", runName.c_str(), outputBufferSize);
		runFile.close();
	}
	delete outputBuffer;
}

SortedRecordRenderer * SortIterator::_externalSort ()
{
	vector<string> runNames;
	_createInitialRuns(runNames); // runNames is modified in this function, as it is passed by reference
	SortedRecordRenderer * renderer = new ExternalRenderer(runNames, _plan->_size, SSD_PAGE_SIZE, MEMORY_SIZE); // runNames is passed by value
	return renderer;
}