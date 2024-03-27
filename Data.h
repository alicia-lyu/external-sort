#pragma once

#include "defs.h"
#include <memory>

typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: 

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

class MemoryRun // an in-memory run of rows
{
public:
    u_int16_t count; // max: 100 MB / 20 B = 5 * 10^3 = 2^13
    RowSize size;
    MemoryRun (u_int16_t count, RowSize size);
    ~MemoryRun ();
    Row * const getRow (u_int16_t index);
    // bool fillRowRandomly(u_int16_t index);
private:
    Row * _rows;
};