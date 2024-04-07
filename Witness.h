#include "Iterator.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const inputPlan);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _inputPlan;
}; // class WitnessPlan

class WitnessIterator : public Iterator
{
public:
    u_int16_t parity; // max: 2000 (record_size) * 4 bits = 2^15
	WitnessIterator (WitnessPlan const * const plan);
	~WitnessIterator ();
	bool next ();
private:
	WitnessPlan const * const _plan;
	Iterator * const _inputIterator;
	RowCount _consumed, _produced;
    std::string getParityString ();
}; // class WitnessIterator
