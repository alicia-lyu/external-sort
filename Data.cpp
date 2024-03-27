#include "Data.h"

Row::Row (RowSize size):
    size (size)
{
    TRACE (true);
    _bytes = (byte *) malloc(size);
}

Row::~Row ()
{
    TRACE (true);
    free(_bytes);
}

byte * Row::begin ()
{
    return _bytes;
}

byte * Row::end ()
{
    return _bytes + size;
}

byte * Row::data ()
{
    return _bytes;
}

MemoryRun::MemoryRun (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (true);
    _rows = (Row *) malloc(sizeof(RowSize) * count);
}

MemoryRun::~MemoryRun ()
{
    TRACE (true);
}

Row * const MemoryRun::getRow (u_int16_t index)
{
    return _rows + index;
}

// bool MemoryRun::fillRowRandomly (u_int16_t index)