#include "Iterator.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const input);
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
    const row_ptr getRow();
private:
	WitnessPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
    row_ptr _row;
    std::string getParityString ();
}; // class WitnessIterator
