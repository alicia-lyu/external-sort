#include "Sort.h"
#include "utils.h"

SortPlan::SortPlan (Plan * const input, u_int32_t recordCountPerRun, RowSize const size) : 
	_input (input), _size (size), _countPerRun(recordCountPerRun)
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
	// Second: Out-of-cache but in-memory sort. Build a small tournament tree with one record from each cache run. In each next() call, tree levels is log(m), m being the number of cache runs. log(n/m)
	std::vector<byte *> rows;
	while (_consumed++ < _plan->_countPerRun) {
		byte * received = _input->next ();
		if (received == nullptr) break;
		rows.push_back(received);
	}
	_tree = new TournamentTree(rows, _plan->_size);
	// TODO: External sort
	_tree->printTree();
	traceprintf ("consumed %lu rows\n",
			(unsigned long) (_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (true);
	delete _input;
	delete _tree;
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // SortIterator::~SortIterator

byte * SortIterator::next ()
{
	// In-memory sort: Return sorted rows
	byte * row = _tree->poll();
	if (row == nullptr) return nullptr;
	++ _produced;
	// traceprintf ("#%d produced %s\n", _produced, rowToHexString(row, _plan->_size).c_str());
	return row;
} // SortIterator::next
