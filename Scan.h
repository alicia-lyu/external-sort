#include <random>
#include <vector>
#include <fstream>

#include "Iterator.h"
#include "utils.h"
#include "defs.h"

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, byte>;

using row = std::vector<byte>; // As class for data records

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
	row getRow();
private:
	ScanPlan const * const _plan;
	RowCount _count;
	row _row;
	random_bytes_engine _engine;
}; // class ScanIterator