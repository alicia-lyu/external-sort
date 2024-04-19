#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const input, RowSize const size) : 
    _input (input), _size (size)
{
	TRACE (false);
} // WitnessPlan::WitnessPlan

WitnessPlan::~WitnessPlan ()
{
	TRACE (false);
	delete _input;
} // WitnessPlan::~WitnessPlan

Iterator * WitnessPlan::init () const
{
	TRACE (false);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()), _consumed (0), _produced (0)
{
	TRACE (false);
    parity = new byte[_plan->_size];
    for (RowSize i = 0; i < _plan->_size; ++i) {
        parity[i] = 0xFF;
    }

    #if defined(VERBOSEL2) || defined(VERBOSEL1)
    traceprintf ("Initialized parity with size %d: %s\n", 
        _plan->_size, rowToHexString(parity, _plan->_size).c_str());
    #endif
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _input;
    traceprintf ("Final parity %s\n", rowToHexString(parity, _plan->_size).c_str());

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // WitnessIterator::~WitnessIterator

byte * WitnessIterator::next ()
{
	TRACE (false);

    byte * received = _input->next ();
    if (received == nullptr) {
        return nullptr;
    } else {
        byte * row = received;
        ++ _consumed;
        for (u_int32_t i = 0; i < _plan->_size; ++i) {
            parity[i] ^= row[i];
        }
        ++ _produced;
        return row;
    }
} // WitnessIterator::next