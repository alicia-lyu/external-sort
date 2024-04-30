#pragma once

#include "Iterator.h"
#include "Buffer.h"

class WitnessPlan : public Plan
{
	friend class WitnessIterator;
public:
	WitnessPlan (Plan * const input, RowSize const size, bool final = false);
	~WitnessPlan ();
	Iterator * init () const;
private:
	Plan * const _input;
	RowSize const _size;
	bool const _final;
}; // class WitnessPlan

class WitnessIterator : public Iterator
{
public:
	byte * parity; // max. 16000
	WitnessIterator (WitnessPlan const * const plan);
	~WitnessIterator ();
	byte * next ();
	bool forceFlushBuffer ();
private:
	WitnessPlan const * const _plan;
	Iterator * const _input;
	RowCount _consumed, _produced;
}; // class WitnessIterator
