#include "Iterator.h"
#include "TournamentTree.h"
#include <fstream>

class SortedRecordRenderer
{
public:
	SortedRecordRenderer (TournamentTree * tree, std::vector<TournamentTree *> cacheTrees, std::vector<std::ifstream> inFileRuns);
	~SortedRecordRenderer ();
	byte * next ();
	void print();
private:
	TournamentTree * _tree;
	std::vector<TournamentTree *> _cacheTrees;
	std::vector<std::ifstream> _inFileRuns;
};

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, u_int32_t recordCountPerRun, RowSize const size, RowCount const count);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	RowCount const _count;
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
	SortedRecordRenderer * _renderer;
	SortedRecordRenderer * _formInMemoryRenderer (); // Returns the tree where the top node is the smallest
	std::vector<string> _createInitialRuns (); // Returns the names of the files created
	SortedRecordRenderer * _mergeRuns (std::vector<string> runNames);
}; // class SortIterator