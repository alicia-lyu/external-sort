#include <limits>
#include "utils.h"
#include "Witness.h"

WitnessPlan::WitnessPlan (Plan * const input, MemoryRun * run, RowSize const size) : 
    _input (input), _run (run), _size (size)
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
	return new WitnessIterator (this, _run, _size);
} // WitnessPlan::init

WitnessIterator::WitnessIterator (WitnessPlan const * const plan, MemoryRun * run, RowSize const size) :
	parity (size, true), _plan (plan), _input (plan->_input->init ()), _run (run), _size (size), _consumed (0), _produced (0)
{
	TRACE (true);
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

    if (!_input->next()) {
        traceprintf ("Final parity %s\n", getParityString().c_str());
        return false;
    } else {
        byte * row = _run->getRow(_consumed);
        return true;
    }
} // WitnessIterator::next

std::string WitnessIterator::getParityString ()
{
    std::string result;
    for (bool b : parity) {
        result += b ? "1" : "0";
    }
    return result;
}