#include "Data.h"

// initialize the static vector
std::vector<MemoryRun *> MemoryRun::memoryRuns;

MemoryRun::MemoryRun (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (true);
    // there are `count` rows, each of size `size`
    // so total memory needed is count * size (bytes)
    _rows = (byte *) malloc(count * size * sizeof(byte));

    // in production, seed the engine with a random number
    // in testing, use the default state of the engine
    // _engine.seed(_device());

    // store this memoryrun object in a static vector
    // so that it can be accessed globally
    MemoryRun::memoryRuns.push_back(this);
    // traceprintf("MemoryRun object at %p created\n", this);
}

MemoryRun::~MemoryRun ()
{
    TRACE (true);
    free(_rows);

    // find which memoryrun object is this in the static vector
    auto it = std::find(MemoryRun::memoryRuns.begin(), MemoryRun::memoryRuns.end(), this);
    // remove this memoryrun object from the static vector
    MemoryRun::memoryRuns.erase(it);
    // traceprintf("MemoryRun object at %p deleted\n", this);
}

byte * MemoryRun::getRow (u_int16_t index)
{
    // to get the row at index, we need to skip (index * size) bytes
    return _rows + index * size;
}

byte * MemoryRun::fillRowRandomly (u_int16_t index)
{
    byte * beginByte = getRow(index);
    byte * endByte = beginByte + size;
    std::generate(beginByte, endByte, std::ref(_engine));
    return beginByte;
}