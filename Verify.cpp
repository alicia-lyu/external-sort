#include "Verify.h"
#include "utils.h"

VerifyPlan::VerifyPlan(Plan * const input, RowSize const size)
    : _input(input), _size(size)
{
} // VerifyPlan::VerifyPlan

VerifyPlan::~VerifyPlan()
{
    // delete the input plan as in the chain deletion
    delete _input;
} // VerifyPlan::~VerifyPlan

Iterator * VerifyPlan::init() const
{
    return new VerifyIterator(this);
} // VerifyPlan::init

VerifyIterator::VerifyIterator(VerifyPlan const * const plan)
    : _plan(plan), _input(_plan->_input->init()), _consumed(0), _produced(0)
{
    lastRow = nullptr;
    isSorted = true;
} // VerifyIterator::VerifyIterator

VerifyIterator::~VerifyIterator()
{
    TRACE (false);

    delete _input;
    traceprintf("Result is %s\n", isSorted ? "sorted" : "not sorted");

    traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_produced),
			(unsigned long) (_consumed));
} // VerifyIterator::~VerifyIterator

byte * VerifyIterator::next()
{
    TRACE (false);

    byte * received = _input->next();
    if (received == nullptr) {
        return nullptr;
    } else {
        byte * row = received;
        ++ _consumed;
        if (lastRow != nullptr) {
            if (memcmp(lastRow, row, _plan->_size) > 0) {
                // TODO: Add in-stream duplicate removal
                isSorted = false;
            }
        }
        lastRow = row;
        ++ _produced;

        #ifdef VERBOSEL2
        traceprintf ("#%llu consumed %s\n", _consumed, rowToString(received, _plan->_size).c_str());
        #endif
        
        return row;
    }
} // VerifyIterator::next