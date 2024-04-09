#include <vector>
#include "Iterator.h"
#include "Data.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const input, RowSize const size);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
}; // class WitnessPlan

class WitnessIterator : public Iterator
{
public:
	byte * parity; // max. 16000
	WitnessIterator (WitnessPlan const * const plan);
	~WitnessIterator ();
	byte * next ();
private:
	WitnessPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
}; // class WitnessIterator
