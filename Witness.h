#include "Iterator.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
    bool const final;
	WitnessPlan (Plan * const input, bool const final);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
}; // class WitnessPlan

class WitnessIterator : public Iterator
{
public:
    u_int16_t parity; // max: 2000 (record_size) * 4 bits = 2^15
	WitnessIterator (WitnessPlan const * const plan);
	~WitnessIterator ();
	bool next ();
    Row * getRow();
private:
	WitnessPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
    Row * _row;
    std::string getParityString ();
}; // class WitnessIterator
