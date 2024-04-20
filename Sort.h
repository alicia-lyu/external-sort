#include "Iterator.h"
#include "TournamentTree.h"
#include "SortedRecordRenderer.h"
#include <fstream>
#include <vector>
#include <map>

using std::vector;
using std::map;

class SortPlan : public Plan
{
	friend class SortIterator;
public:
	SortPlan (Plan * const input, RowSize const size, RowCount const count, bool removeDuplicates = false);
	~SortPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	RowCount const _count;
	u_int32_t _recordCountPerRun; // Changes when spill to HDD
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
	SortedRecordRenderer * _formInMemoryRenderer (RowCount base = 0, u_int16_t runNumber = 0); // Returns the tree where the top node is the smallest
	vector<string> _createInitialRuns (); // Returns the names of the files created
	SortedRecordRenderer * _externalSort ();
	u_int8_t profileReadAheadBuffers (vector<string>& runNames, u_int16_t mergedRunCount); // return the number of read-ahead buffers of HDD page size
}; // class SortIterator