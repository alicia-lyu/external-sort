#pragma once

#include "Iterator.h"
#include "Buffer.h"
#include <fstream>

using std::ofstream;

class OutputPlan : public Plan
{
    friend class OutputIterator;
public:
    OutputPlan (Plan * const input, RowSize const size, const string &outputPath);
    ~OutputPlan ();
    Iterator * init () const;
private:
    Plan * const _input;
    RowSize const _size;
    const string & _outputPath;
}; // class OutputPlan

class OutputIterator : public Iterator
{
public:
    OutputIterator (OutputPlan const * const plan);
    ~OutputIterator ();
    byte * next ();
private:
    OutputPlan const * const _plan;
    Iterator * const _input;
    ofstream _outputFile;
    RowCount _consumed, _produced;
}; // class OutputIterator