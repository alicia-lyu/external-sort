#include <vector>
#include <fstream>

#include "Iterator.h"
#include "utils.h"
#include "defs.h"
#include "Data.h"

class ScanPlan : public Plan
{
	friend class ScanIterator;
public:
	ScanPlan (RowCount const count, RowSize const size, ofstream_ptr const inFile);
	~ScanPlan ();
	Iterator * init () const;
private:
	RowCount const _count;
	RowSize const _size;
	ofstream_ptr const _inFile;
}; // class ScanPlan

class ScanIterator : public Iterator
{
public:
	ScanIterator (ScanPlan const * const plan);
	~ScanIterator ();
	bool next ();
	row_ptr const getRow(); // returned pointer cannot be directed to another object
private:
	ScanPlan const * const _plan;
	RowCount _count;
	row_ptr _row; // _row pointer changes reference every step
}; // class ScanIterator