#include "Data.h"

MemoryRun::MemoryRun (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (true);
    _rows = (byte **) malloc(sizeof(RowSize) * count);
}

MemoryRun::~MemoryRun ()
{
    TRACE (true);
    free(_rows);
}

byte * const MemoryRun::getRow (u_int16_t index)
{
    return _rows[index];
}

byte * MemoryRun::fillRowRandomly (u_int16_t index)
{
    byte * beginByte = getRow(index);
    byte * endByte = beginByte + size;
    std::generate(beginByte, endByte, std::ref(_engine));
    return beginByte;
}