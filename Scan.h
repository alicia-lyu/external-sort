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
	ScanPlan (RowCount const count, RowSize const size, u_int32_t recordCountPerRun, ofstream_ptr const inFile);
	~ScanPlan ();
	Iterator * init () const;
private:
	RowCount const _count;
	RowSize const _size;
	ofstream_ptr const _inFile;
	u_int32_t const _countPerRun;
}; // class ScanPlan

class ScanIterator : public Iterator
{
public:
	ScanIterator (ScanPlan const * const plan);
	~ScanIterator ();
	byte * next ();
private:
	ScanPlan const * const _plan;
	RowCount _count;
	MemoryRun * _run;
}; // class ScanIterator