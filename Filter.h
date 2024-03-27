#include "Iterator.h"

class FilterPlan : public Plan
{
	friend class FilterIterator;
public:
	FilterPlan (Plan * const input, MemoryRun * run);
	~FilterPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	MemoryRun * _run;
}; // class FilterPlan

class FilterIterator : public Iterator
{
public:
	FilterIterator (FilterPlan const * const plan, MemoryRun * run);
	~FilterIterator ();
	bool next ();
private:
	FilterPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
	MemoryRun * _run;
}; // class FilterIterator
