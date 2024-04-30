#include "Iterator.h"
#include "TournamentTree.h"
#include "SortedRecordRenderer.h"
#include <fstream>
#include <vector>
#include <map>

using std::vector;
using std::map;
using std::tuple;

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, RowSize const size, RowCount const count, bool removeDuplicates = false, std::function<void ()> forceFlushBufferHandler = nullptr);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	RowCount const _count;
	bool _removeDuplicates;
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
	SortedRecordRenderer * _formInMemoryRenderer (RowCount base = 0, u_int16_t runNumber = 0, 
		u_int32_t memory_limit = 0,
		bool materialize = true); // Returns the tree where the top node is the smallest
	vector<string> _createInitialRuns (); // Returns the names of the files created
	SortedRecordRenderer * gracefulDegradation ();
	SortedRecordRenderer * _externalSort (); // Returns the renderer that is ready to render the next row
}; // class SortIterator