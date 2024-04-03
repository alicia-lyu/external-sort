#include "Data.h"

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
}

MemoryRun::~MemoryRun ()
{
    TRACE (true);
    free(_rows);
}

byte * MemoryRun::getRow (u_int16_t index)
{
    // to get the row at index, we need to skip (index * size) bytes
    return _rows;
}

byte * MemoryRun::fillRowRandomly (u_int16_t index)
{
    byte * beginByte = getRow(index);
    byte * endByte = beginByte + size;
    std::generate(beginByte, endByte, std::ref(_engine));
    return beginByte;
}