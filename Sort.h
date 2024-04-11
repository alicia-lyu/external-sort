#include "Iterator.h"
#include "TournamentTree.h"

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
	TournamentTree * _tree;
	TournamentTree * _formInMemoryTree (); // Returns the tree where the top node is the smallest
	std::vector<string> _createInitialRuns (); // Returns the names of the files created
}; // class SortIterator
