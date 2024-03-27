#include <random>
#include <vector>
#include <fstream>

#include "Iterator.h"
#include "utils.h"
#include "defs.h"
#include "Row.h"

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, byte>;

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
	Row * getRow();
private:
	ScanPlan const * const _plan;
	RowCount _count;
	Row * _row;
	random_bytes_engine _engine;
}; // class ScanIterator