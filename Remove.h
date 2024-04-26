#pragma once

#include "Iterator.h"
#include "Buffer.h"

/*
This class performe the in-stream duplicate removal
It reads the input stream and compares with previous row
If the row is equal to the previous row, it is removed
and the next row is read and checked, until a different row is found
and this row is returned
*/

class InStreamRemovePlan : public Plan
{
    friend class InStreamRemoveIterator;
public:
    InStreamRemovePlan (Plan * const input, RowSize const size, bool activated);
    ~InStreamRemovePlan ();
    Iterator * init () const;
private:
    Plan * const _input;
    RowSize const _size;
    // need to compare between two different removal methods
    // so if we are using the in-sort removal, we do not activate it
    // and the iterator simply returns the input rows
    bool const _activated;
}; // class InStreamRemovePlan

class InStreamRemoveIterator : public Iterator
{
public:
    byte * lastRow;
    InStreamRemoveIterator (InStreamRemovePlan const * const plan);
    ~InStreamRemoveIterator ();
    byte * next ();
private:
    InStreamRemovePlan const * const _plan;
    Iterator * const _input;
    RowCount _consumed, _produced;
    RowCount _removed;
}; // class InStreamRemoveIterator