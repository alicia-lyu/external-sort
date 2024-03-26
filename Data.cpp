#include "Data.h"

DataPlan::DataPlan (ByteCount const byteCount):
    _byteCount (byteCount)
{
    TRACE (true);
}

DataPlan::~DataPlan ()
{
    TRACE (true);
}

Iterator * DataPlan::init () const
{
    TRACE (true);
    return new DataIterator (this);
}

DataIterator::DataIterator (DataPlan const * const plan):
    _plan (plan), _count (0)
{
    TRACE (true);
}

DataIterator::~DataIterator ()
{
    TRACE (true);
    traceprintf ("produced %lu of %lu bytes\n",
        (unsigned long) (_count),
        (unsigned long) (_plan->_byteCount)
    );
}

bool DataIterator::next ()
{
    TRACE (true);

    if (_count >= _plan->_byteCount)
        return false;

    ++ _count;
    return true;
}

byte DataIterator::getByte ()
{
    
}