#include "Row.h"

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