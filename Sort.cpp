#include "Sort.h"
#include "utils.h"
#include <stdexcept>

SortedRecordRenderer::SortedRecordRenderer (
	TournamentTree * tree, 
	std::vector<TournamentTree *> cacheTrees, 
	std::vector<std::ifstream> inFileRuns) :
	_tree (tree), _cacheTrees (cacheTrees), _inFileRuns (inFileRuns)
{
	TRACE (true);
	if (_cacheTrees.size() != 0 && _inFileRuns.size() != 0) {
		throw std::invalid_argument("Only either cache trees or in-file runs can be provided");
	}
	this->print();
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (true);
	delete _tree;
	for (auto cacheTree : _cacheTrees) {
		delete cacheTree;
	}
} // SortedRecordRenderer::~SortedRecordRenderer

byte * SortedRecordRenderer::next ()
{
	if (_cacheTrees.size() == 0 && _inFileRuns.size() == 0) {
		// No cache trees, we only have one huge tree for all records in memory
		return _tree->poll();
	} else if (_inFileRuns.size() == 0) {
		// We have cache trees, we have to merge them along the way of polling the huge tree
		u_int8_t bufferNum = _tree->peek();
		TournamentTree * cacheTree = _cacheTrees.at(bufferNum);
		byte * row = cacheTree->poll();
		if (row == nullptr) {
			return _tree->poll();
		} else {
			return _tree->pushAndPoll(row);
		}
	} else {
		// TODO: We have in-file runs, we have to merge them along the way of polling the huge tree
		return nullptr;
	}
} // SortedRecordRenderer::next

void SortedRecordRenderer::print ()
{
	traceprintf ("%d cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // SortedRecordRenderer::print

SortPlan::SortPlan (Plan * const input, u_int32_t recordCountPerRun, RowSize const size, RowCount const count) : 
	_input (input), _size (size), _countPerRun(recordCountPerRun), _count (count)
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
		_renderer = _formInMemoryRenderer();
	} else {
		std::vector<string> runNames = _createInitialRuns();
		_renderer = _mergeRuns(runNames);
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
	// In-memory sort: Return sorted rows
	byte * row = _renderer->next();
	if (row == nullptr) return nullptr;
	++ _produced;
	// traceprintf ("#%d produced %s\n", _produced, rowToHexString(row, _plan->_size).c_str());
	return row;
} // SortIterator::next


SortedRecordRenderer * SortIterator::_formInMemoryRenderer ()
{
	std::vector<byte *> rows;
	while (_consumed++ < _plan->_count) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		rows.push_back(received);
	}
	// TODO: break rows into cache lines
	// Build a tree for each cache line, log (n/m) levels, 
	// Then build a tree for the root nodes of the cache line trees, log (m) levels
	// n being the number of records in memory, m being the number of cache lines
	TournamentTree * tree = new TournamentTree(rows, _plan->_size);
	std::vector<TournamentTree *> cacheTrees = {};
	std::vector<std::ifstream> inFileRuns = {};
	SortedRecordRenderer * renderer = new SortedRecordRenderer(tree, cacheTrees, inFileRuns);
	return renderer;
}

std::vector<string> SortIterator::_createInitialRuns ()
{
	std::vector<string> runNames;
	u_int32_t outputBufferSize = _plan->_countPerRun * _plan->_size; // max. 100 MB = 2^27
	byte * outputBuffer = new byte[outputBufferSize];
	while (_consumed < _plan->_count) {
		string runName = "run" + std::to_string(_consumed / _plan->_countPerRun);
		runNames.push_back(runName);
		traceprintf ("Creating run file %s\n", runName.c_str());
		SortedRecordRenderer * renderer = _formInMemoryRenderer();
		while (true) {
			byte * row = renderer->next();
			if (row == nullptr) break;
			memcpy(outputBuffer, row, _plan->_size);
			outputBuffer += _plan->_size;
		}
		outputBuffer -= outputBufferSize;
		std::ofstream runFile(runName, std::ios::binary);
		runFile.write(reinterpret_cast<char *>(outputBuffer), outputBufferSize);
		runFile.close();
	}
	return runNames;
}

SortedRecordRenderer * SortIterator::_mergeRuns (std::vector<string> runNames)
{
	u_int16_t flashPageSize = 20000; // 20 KB, 2^16 = 64 KB
	// TODO
	return nullptr;
}