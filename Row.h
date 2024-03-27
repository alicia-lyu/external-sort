#pragma once

#include "defs.h"
#include <memory>

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

using row_ptr = std::shared_ptr<Row>;