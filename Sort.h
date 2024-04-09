#include "Iterator.h"

// TODO: Split into internal sort and external sort
// NOW:
// Build a huge tournament tree on all records in memory, next() returns the smallest record. Blocks all upstream next() calls until the heapification is done. With each next() after the first, tree levels is log(n), n being the number of records in memory.
// LATER:
// First: In-cache sort (a separate method to be called when next() is first called). Must happen in place, otherwise it will spill outside of cache line. This must finish before next() can return anything. Blocks all upstream next() calls.
// Second: Out-of-cache but in-memory sort. Allocate a separate output buffer, whose size is the final size of a memory run stored in SSD. This stage can be pipelined with the rest of the program. With each next() after the first, tree levels is log(m), m being the number of cache runs.
class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, u_int32_t recordCountPerRun, RowSize const size);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	u_int32_t const _countPerRun;
}; // class SortPlan

class SortIterator : public Iterator
{
public:
	SortIterator (SortPlan const * const plan);
	~SortIterator ();
	byte * next ();
private:
	SortPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
}; // class SortIterator
