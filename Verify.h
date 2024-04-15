#pragma once

#include "Iterator.h"
#include "Buffer.h"

class VerifyPlan : public Plan
{
    friend class VerifyIterator;
public:
    VerifyPlan (Plan * const input, RowSize const size);
    ~VerifyPlan ();
    Iterator * init () const;
private:
    Plan * const _input;
    RowSize const _size;
}; // class VerifyPlan

class VerifyIterator : public Iterator
{
public:
    byte * lastRow;
    VerifyIterator (VerifyPlan const * const plan);
    ~VerifyIterator ();
    byte * next ();
private:
    VerifyPlan const * const _plan;
    Iterator * const _input;
    RowCount _consumed, _produced;
    bool isSorted;
}; // class VerifyIterator