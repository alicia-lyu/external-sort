#include "Filter.h"

FilterPlan::FilterPlan (Plan * const input, MemoryRun * run) : 
	_input (input), _run (run)
{
	TRACE (true);
} // FilterPlan::FilterPlan

FilterPlan::~FilterPlan ()
{
	TRACE (true);
	delete _input;
} // FilterPlan::~FilterPlan

Iterator * FilterPlan::init () const
{
	TRACE (true);
	return new FilterIterator (this, _run);
} // FilterPlan::init

FilterIterator::FilterIterator (FilterPlan const * const plan, MemoryRun * run) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);
} // FilterIterator::FilterIterator

FilterIterator::~FilterIterator ()
{
	TRACE (true);

	delete _input;

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // FilterIterator::~FilterIterator

byte * FilterIterator::next ()
{
	TRACE (true);

	do
	{
		byte * received=_input->next ();
		if ( received == nullptr)  return nullptr;
		++ _consumed;
	} while (_consumed % 2 == 0);

	++ _produced;
	return nullptr;
} // FilterIterator::next
