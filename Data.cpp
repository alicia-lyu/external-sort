#include "Data.h"

MemoryRun::MemoryRun (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (true);
    _rows = (byte *) malloc(size * count * sizeof(byte));
}

MemoryRun::~MemoryRun ()
{
    TRACE (true);
    free(_rows);
}

byte * MemoryRun::getRow (u_int16_t index)
{
    return _rows + index * size;
}

byte * MemoryRun::fillRowRandomly (u_int16_t index)
{
    byte * beginByte = getRow(index);
    byte * endByte = beginByte + size;
    std::generate(beginByte, endByte, std::ref(_engine));
    return beginByte;
}