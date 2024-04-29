#include "Remove.h"
#include "utils.h"

InStreamRemovePlan::InStreamRemovePlan (Plan * const input, RowSize const size, bool activated)
    : _input(input), _size(size), _activated(activated)
{
    TRACE(false);
    if (_activated) {
        #ifdef PRODUCTION
        Trace::PrintTrace(OP_STATE, INIT_SORT, "Remove duplicates using in-stream method");
        #endif
    }
} // InStreamRemovePlan::InStreamRemovePlan

InStreamRemovePlan::~InStreamRemovePlan()
{
    TRACE(false);
    // delete the input plan as in the chain deletion
    delete _input;
} // InStreamRemovePlan::~InStreamRemovePlan

Iterator * InStreamRemovePlan::init() const
{
    TRACE(false);
    return new InStreamRemoveIterator(this);
} // InStreamRemovePlan::init

InStreamRemoveIterator::InStreamRemoveIterator(InStreamRemovePlan const * const plan)
    : _plan(plan), _input(_plan->_input->init()), _consumed(0), _produced(0), _removed(0)
{
    TRACE(false);
} // InStreamRemoveIterator::InStreamRemoveIterator

InStreamRemoveIterator::~InStreamRemoveIterator()
{
    TRACE(false);
    delete _input;
    if (_plan->_activated) {
        #if defined(VERBOSEL2) || defined(VERBOSEL1)
        traceprintf("Removed %lu rows\n", (unsigned long)(_removed));
        traceprintf("produced %lu of %lu rows\n",
                    (unsigned long)(_produced),
                    (unsigned long)(_consumed));
        #endif
    }
        
} // InStreamRemoveIterator::~InStreamRemoveIterator

byte * InStreamRemoveIterator::next()
{
    TRACE(false);

    // if not activated, simply return the input rows
    if (!_plan->_activated) {
        return _input->next();
    }

    while (true) {
        byte * received = _input->next();
        if (received == nullptr) {
            return nullptr;
        } else {
            ++_consumed;
            if (lastRow != nullptr) {
                if (memcmp(lastRow, received, _plan->_size) == 0) {
                    // Remove the row
                    ++_removed;

                    #ifdef VERBOSEL2
                    traceprintf ("#%lu removed with value %s\n",
                        (unsigned long) (_consumed), rowToString(received, _plan->_size).c_str());
                    #endif

                    continue;
                }
            }
            lastRow = received;
            ++_produced;
            return received;
        }
    }

    // this line will never be reached
    return nullptr;
}