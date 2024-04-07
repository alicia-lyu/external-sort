#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const input, MemoryRun * run, RowSize const size) : 
    _input (input), _run (run), _size (size)
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
	return new WitnessIterator (this, _run, _size);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan, MemoryRun * run, RowSize const size) :
	_plan (plan), _input (plan->_input->init ()), _run (run), _size (size), _consumed (0), _produced (0)
{
	TRACE (false);
    parity = new byte[size];
    for (RowSize i = 0; i < size; ++i) {
        parity[i] = 0xFF;
    }
    traceprintf ("Initialized parity %s\n", rowToHexString(parity, _size).c_str());
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (false);

	delete _input;

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next ()
{
	TRACE (false);

    if (!_input->next()) {
        traceprintf ("Final parity %s\n", rowToHexString(parity, _size).c_str());
        return false;
    } else {
        byte * row = _run->getRow(_consumed);
        ++ _consumed;
        for (u_int32_t i = 0; i < _size; ++i) {
            parity[i] ^= row[i];
        }
        ++ _produced;
        return true;
    }
} // WitnessIterator::next