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
    free(_rows);
}

Row * const MemoryRun::getRow (u_int16_t index)
{
    return _rows + index;
}

Row * MemoryRun::fillRowRandomly (u_int16_t index)
{
    Row * row = getRow(index);
    std::generate(row->begin(), row->end(), std::ref(_engine));
    return row;
}