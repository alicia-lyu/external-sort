#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const input) : 
    _input (input)
{
	TRACE (true);
} // WitnessPlan::WitnessPlan

WitnessPlan::~WitnessPlan ()
{
	TRACE (true);
	delete _input;
} // WitnessPlan::~WitnessPlan

Iterator * WitnessPlan::init () const
{
	TRACE (true);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _input (plan->_input->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);
    parity = 0xFFF;
    // TODO: final witness allocate output buffer, or add a new plan to do that
    traceprintf ("Initialized parity %s\n", getParityString().c_str());
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (true);

	delete _input;

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next ()
{
	TRACE (true);

    if (_produced >= _consumed && !_input->next()) {
        traceprintf ("Final parity %s\n", getParityString().c_str());
        return false;
    } else {
        // TODO: get row from MemoryRun
        // _row = _input->getRow();
        // byte * rowContent = _row->data();
        // parity ^= *rowContent;
        // ++_produced;
        return true;
    }
} // WitnessIterator::next

std::string WitnessIterator::getParityString ()
{
    std::string parityString = rowToHexString((byte *) &parity, (u_int16_t) 4);
    return parityString;
}