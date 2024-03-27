#pragma once

#include "defs.h"

typedef u_int16_t RowSize; // 20-2000, unit: byte

class Row
{
public:
    RowSize size;
private:
    byte * _bytes;
public:
    Row (RowSize size);
    ~Row ();
    byte * begin ();
    byte * end ();
    byte * data ();
};