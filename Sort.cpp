#include "Sort.h"
#include "utils.h"
#include "ExternalRenderer.h"
#include <stdexcept>

SortPlan::SortPlan (Plan * const input, u_int32_t recordCountPerRun, RowSize const size, RowCount const count) : 
	_input (input), _size (size), _count (count), _countPerRun(recordCountPerRun)
{
	TRACE (true);
} // SortPlan::SortPlan

SortPlan::~SortPlan ()
{
	TRACE (true);
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
	// In-memory sort: Read all rows from input, form a tournament tree that is ready to render the next record
	// NOW:
	// Build a huge tournament tree on all records in memory, In each next() call, tree levels is log(n), n being the number of records in memory.
	// LATER:
	// First: In-cache sort. Must happen in place, otherwise it will spill outside of cache line.
	// Second: Out-of-cache but in-memory sort. Build a small tournament tree with one record from each cache run. In each next() call, tree levels is log(m), m being the number of cache runs.
	if (_plan->_count <= _plan->_countPerRun) {
		traceprintf ("%llu records fit in memory (%d)\n", _plan->_count, _plan->_countPerRun);
		_renderer = _formInMemoryRenderer();
	} else {
		traceprintf ("%llu records do not fit in memory (%d)\n", _plan->_count, _plan->_countPerRun);
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
	if (row == nullptr) return nullptr;
	++ _produced;
	traceprintf ("#%llu produced %s\n", _produced, rowToHexString(row, _plan->_size).c_str());
	return row;
} // SortIterator::next


SortedRecordRenderer * SortIterator::_formInMemoryRenderer (RowCount base)
{
	std::vector<byte *> rows;
	while (_consumed - base < _plan->_countPerRun) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		rows.push_back(received);
		++ _consumed;
	}
	traceprintf ("Forming in-memory renderer with %zu rows\n", rows.size());
	// TODO: break rows into cache lines
	// Build a tree for each cache line, log (n/m) levels, 
	// Then build a tree for the root nodes of the cache line trees, log (m) levels
	// n being the number of records in memory, m being the number of cache lines
	TournamentTree * tree = new TournamentTree(rows, _plan->_size);
	SortedRecordRenderer * renderer = new NaiveRenderer(tree);
	return renderer;
}

std::vector<string> SortIterator::_createInitialRuns ()
{
	std::vector<string> runNames;
	Buffer * outputBuffer = new Buffer(_plan->_countPerRun, _plan->_size);
	while (_consumed < _plan->_count) {
		outputBuffer->reset();
		string runName = std::string(".") + SEPARATOR + std::string("spills") + SEPARATOR + std::string("pass0") + SEPARATOR + std::string("run") + std::to_string(_consumed / _plan->_countPerRun) + std::string(".bin");
		runNames.push_back(runName);
		SortedRecordRenderer * renderer = _formInMemoryRenderer(_consumed);
		int i;
		for (i = 0; i < _plan->_countPerRun; i++) {
			byte * row = renderer->next();
			if (row == nullptr) break;
			if (outputBuffer->copy(row) == nullptr) {
				throw std::runtime_error("Output buffer overflows when creating initial run " + runName + ".\n");
			}
			// traceprintf ("#%d produced %s for runFile %s\n", i, rowToHexString(row, _plan->_size).c_str(), runName.c_str());
		}
		std::ofstream runFile(runName, std::ios::binary);
		int outputBufferSize = i * _plan->_size;
		runFile.write(reinterpret_cast<char *>(outputBuffer->data()), outputBufferSize);
		traceprintf ("Created run file %s with size %d\n", runName.c_str(), outputBufferSize);
		runFile.close();
	}
	delete outputBuffer;
	return runNames;
}

SortedRecordRenderer * SortIterator::_mergeRuns (std::vector<string> runNames)
{
	traceprintf ("Merging %zu runs\n", runNames.size());
	u_int16_t flashPageSize = 20; // 20 KB, 2^16 = 64 KB
	// TODO: Multi-level merge
	SortedRecordRenderer * renderer = new ExternalRenderer(runNames, _plan->_size, flashPageSize);
	return renderer;
}

SortedRecordRenderer * SortIterator::_externalSort ()
{
	std::vector<string> runNames = _createInitialRuns();
	return _mergeRuns(runNames);
}