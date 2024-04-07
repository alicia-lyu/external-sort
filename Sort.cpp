#include "Sort.h"

SortPlan::SortPlan (Plan * const input, MemoryRun * run, RowSize const size) : 
	_input (input), _run (run), _size (size)
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
	return new SortIterator (this, _run, _size);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan, MemoryRun * run, RowSize const size) :
	_plan (plan), _input (plan->_input->init ()), _run (run), _size (size), _consumed (0), _produced (0)
{
	TRACE (true);
	traceprintf ("consumed %lu rows\n",
			(unsigned long) (_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (true);
	delete _input;
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // SortIterator::~SortIterator

bool SortIterator::next ()
{
	TRACE (true);

	if (!_input->next()) {
		return false;
	}
	
	++ _produced;
	return true;
} // SortIterator::next
