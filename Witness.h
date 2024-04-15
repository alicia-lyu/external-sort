#pragma once

#include "Iterator.h"
#include "Buffer.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const input, RowSize const size);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _inputPlan;
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
	Iterator * const _inputIterator;
	RowCount _consumed, _produced;
}; // class WitnessIterator
