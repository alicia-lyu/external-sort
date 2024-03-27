#include "Sort.h"

SortPlan::SortPlan (Plan * const input, MemoryRun * run) : 
	_input (input), _run (run)
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
	return new SortIterator (this, _run);
} // SortPlan::init

SortIterator::SortIterator (SortPlan const * const plan, MemoryRun * run) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);

	while (_input->next ())  ++ _consumed;
	delete _input;

	traceprintf ("consumed %lu rows\n",
			(unsigned long) (_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator ()
{
	TRACE (true);

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // SortIterator::~SortIterator

bool SortIterator::next ()
{
	TRACE (true);

	if (_produced >= _consumed)  return false;

	++ _produced;
	return true;
} // SortIterator::next
