#include <vector>
#include "Iterator.h"
#include "Data.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const input, MemoryRun * run, RowSize const size);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	MemoryRun * _run;
	RowSize const _size;
}; // class WitnessPlan

class WitnessIterator : public Iterator
{
public:
	std::vector<bool> parity;
	WitnessIterator (WitnessPlan const * const plan, MemoryRun * run, RowSize const size);
	~WitnessIterator ();
	bool next ();
private:
	WitnessPlan const * const _plan;
	MemoryRun * _run;
	Iterator * const _input;
	RowSize const _size;
	RowCount _consumed, _produced;
    std::string getParityString ();
}; // class WitnessIterator
