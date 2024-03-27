#include <random>
#include <vector>

#include "Iterator.h"

typedef u_int16_t RowSize;

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

using row = std::vector<unsigned char>;

class ScanPlan : public Plan
{
	friend class ScanIterator;
public:
	ScanPlan (RowCount const count, RowSize const size);
	~ScanPlan ();
	Iterator * init () const;
private:
	RowCount const _count;
	RowSize const _size;
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
