#include "Iterator.h"

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, MemoryRun * run);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	MemoryRun * _run;
}; // class SortPlan

class SortIterator : public Iterator
{
public:
	SortIterator (SortPlan const * const plan, MemoryRun * run);
	~SortIterator ();
	bool next ();
private:
	SortPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
}; // class SortIterator
