#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const input, RowSize const size) : 
    _inputPlan (input), _size (size)
{
	TRACE (false);
} // WitnessPlan::WitnessPlan

WitnessPlan::~WitnessPlan ()
{
	TRACE (false);
    // why? We did not create a new object before
	delete _inputPlan;
} // WitnessPlan::~WitnessPlan

Iterator * WitnessPlan::init () const
{
	TRACE (false);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _inputIterator (plan->_inputPlan->init ()), _consumed (0), _produced (0)
{
	TRACE (false);
    parity = new byte[_plan->_size];
    for (RowSize i = 0; i < _plan->_size; ++i) {
        parity[i] = 0xFF;
    }
    // traceprintf ("Initialized parity %s\n", rowToHexString(parity, _plan->_size).c_str());
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _inputIterator;
    traceprintf ("Final parity %s\n", rowToHexString(parity, _plan->_size).c_str());

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // WitnessIterator::~WitnessIterator

byte * WitnessIterator::next ()
{
	TRACE (false);

    byte * received = _inputIterator->next ();
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