#pragma once

#include "Iterator.h"
#include "Buffer.h"

/*
VerifyPlan verifies the sorted stream for:
- whether it is sorted
- whether it has duplicates
*/
class VerifyPlan : public Plan
{
    friend class VerifyIterator;
public:
    VerifyPlan (Plan * const input, RowSize const size, bool descending = false);
    ~VerifyPlan ();
    Iterator * init () const;
private:
    Plan * const _input;
    RowSize const _size;
    bool const _descending;
}; // class VerifyPlan

class VerifyIterator : public Iterator
{
public:
    VerifyIterator (VerifyPlan const * const plan);
    ~VerifyIterator ();
    byte * next ();
private:
    byte * lastRow;
    VerifyPlan const * const _plan;
    Iterator * const _input;
    RowCount _consumed, _produced;
    bool isSorted, hasDuplicates;
    bool const _descending;
    bool isFirstRow;
}; // class VerifyIterator