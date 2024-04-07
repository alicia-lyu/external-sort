#include "Iterator.h"

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, MemoryRun * run, RowSize const size);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	MemoryRun * _run;
	RowSize const _size;
}; // class SortPlan

class SortIterator : public Iterator
{
public:
	SortIterator (SortPlan const * const plan, MemoryRun * run, RowSize const size);
	~SortIterator ();
	bool next ();
private:
	SortPlan const * const _plan;
	Iterator * const _input;
	MemoryRun * _run;
	RowSize const _size;
	RowCount _consumed, _produced;
}; // class SortIterator
