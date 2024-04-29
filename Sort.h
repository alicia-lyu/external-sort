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
	SortedRecordRenderer * _formInMemoryRenderer (RowCount base = 0, u_int16_t runNumber = 0, 
		u_int32_t memory_limit = MEMORY_SIZE - SSD_PAGE_SIZE,
		bool materialize = true); // Returns the tree where the top node is the smallest
	vector<string> _createInitialRuns (); // Returns the names of the files created
	SortedRecordRenderer * gracefulDegradation (); // Returns the renderer that is ready to render the next row
	SortedRecordRenderer * _externalSort (); // Returns the renderer that is ready to render the next row
	u_int64_t profileReadAheadSize (vector<string>& runNames, u_int16_t mergedRunCount); // return the number of read-ahead buffers of HDD page size
	tuple<u_int16_t, u_int64_t, u_int64_t> assignRuns(vector<string>& runNames, u_int16_t mergedRunCount); // The first return is updated merged run count [initial merged run count, returned merged run count) is the assigned runs; the second is the read ahead size; the third is all memory consumption till the end of runNames
	SortedRecordRenderer * gracefulMerge (vector<string>& runNames, int basePass, int rendererNum);
}; // class SortIterator