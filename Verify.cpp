#include "Verify.h"
#include <limits>

VerifyPlan::VerifyPlan(Plan * const input, RowSize const size)
    : _inputPlan(input), _size(size)
{
    TRACE(false);
} // VerifyPlan::VerifyPlan

VerifyPlan::~VerifyPlan()
{
    TRACE(false);
} // VerifyPlan::~VerifyPlan

Iterator * VerifyPlan::init() const
{
    TRACE(false);
    return new VerifyIterator(this);
} // VerifyPlan::init

VerifyIterator::VerifyIterator(VerifyPlan const * const plan)
    : _plan(plan), _inputIterator(plan->_inputPlan->init()), _consumed(0), _produced(0)
{
    TRACE(false);
    last_byte = 0;
} // VerifyIterator::VerifyIterator

VerifyIterator::~VerifyIterator()
{
    TRACE(false);

    delete _inputIterator;

    traceprintf("produced %lu of %lu rows\n",
                (unsigned long)(_produced),
                (unsigned long)(_consumed));
} // VerifyIterator::~VerifyIterator

byte * VerifyIterator::next()
{
    TRACE(false);

    byte *received = _inputIterator->next();

    if (received == nullptr)
    {
        return nullptr;
    }
    else
    {
        ++_consumed;


    }
} // VerifyIterator::next