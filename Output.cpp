#include "Output.h"

OutputPlan::OutputPlan(Plan * const input, RowSize const size, const string &outputPath)
    : _input(input), _size(size), _outputPath(outputPath)
{
} // OutputPlan::OutputPlan

OutputPlan::~OutputPlan()
{
    delete _input;
} // OutputPlan::~OutputPlan

Iterator * OutputPlan::init() const
{
    return new OutputIterator(this);
} // OutputPlan::init

OutputIterator::OutputIterator(OutputPlan const * const plan)
    : _plan(plan), _input(plan->_input->init()), _consumed(0), _produced(0)
{
    _outputFile.open(_plan->_outputPath.c_str(), ofstream::out | ofstream::trunc | ofstream::binary);
} // OutputIterator::OutputIterator

OutputIterator::~OutputIterator()
{
    _outputFile.close();
    delete _input;
} // OutputIterator::~OutputIterator

byte * OutputIterator::next()
{
    byte * received = _input->next();
    if (received == nullptr) {
        return nullptr;
    } else {
        byte * row = received;
        ++_consumed;
        _outputFile.write(reinterpret_cast<char *>(row), _plan->_size);
        _outputFile.write("\n", 1);
        ++_produced;
        return row;
    }
} // OutputIterator::next