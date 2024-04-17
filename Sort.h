#include "Iterator.h"
#include "TournamentTree.h"
#include "SortedRecordRenderer.h"
#include <fstream>

using std::vector;

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, RowSize const size, RowCount const count);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	RowCount const _count;
	u_int32_t _recordCountPerRun; // Changes when spill to HDD
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
	SortedRecordRenderer * _renderer;
	SortedRecordRenderer * _formInMemoryRenderer (RowCount base = 0); // Returns the tree where the top node is the smallest
	void _createInitialRuns (vector<string> &runNames); // Returns the names of the files created
	SortedRecordRenderer * _mergeRuns (vector<string> &runNames);
	SortedRecordRenderer * _externalSort ();
}; // class SortIterator