#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const inputPlan) : 
    _inputPlan (inputPlan)
{
	TRACE (true);
} // WitnessPlan::WitnessPlan

WitnessPlan::~WitnessPlan ()
{
	TRACE (true);
	delete _inputPlan;
} // WitnessPlan::~WitnessPlan

Iterator * WitnessPlan::init () const
{
	TRACE (true);
	return new WitnessIterator (this);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan) :
	_plan (plan), _inputIterator (plan->_inputPlan->init ()),
	_consumed (0), _produced (0)
{
	TRACE (true);
    parity = std::numeric_limits<u_int16_t>::max(); // TODO: debug
    traceprintf ("Initialized parity %s\n", getParityString().c_str());
} // WitnessIterator::WitnessIterator

WitnessIterator::~WitnessIterator ()
{
	TRACE (true);

	delete _inputIterator;

	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // WitnessIterator::~WitnessIterator

bool WitnessIterator::next ()
{
	TRACE (true);

    if (_produced >= _consumed && !_inputIterator->next()) {
        traceprintf ("Final parity %s\n", getParityString().c_str());
        return false;
    } else {
        // TODO: design appropriate interface and inheritence
        // to enable getting rows from input iterator
        
        // byte* _row = ...
        // for (u_int16_t i = 0; i < _plan->_inputPlan->getRowSize(); ++i) {
        //     byte rowContent = _row[i];
        //     parity ^= rowContent;
        // }
        // ++_produced;
        // return true;
    }
} // WitnessIterator::next

std::string WitnessIterator::getParityString ()
{
    std::string parityString = rowToHexString((byte *) &parity, (u_int16_t) 4);
    return parityString;
}