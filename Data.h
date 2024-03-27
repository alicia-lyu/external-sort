#pragma once

#include "defs.h"
#include <memory>
#include <random>

typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: 

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, byte>;

class MemoryRun // an in-memory run of rows
{
public:
    u_int16_t count; // max: 100 MB / 20 B = 5 * 10^3 = 2^13
    RowSize size;
    MemoryRun (u_int16_t count, RowSize size);
    ~MemoryRun ();
    byte * getRow (u_int16_t index);
    byte * fillRowRandomly(u_int16_t index);
private:
    byte * _rows;
    random_bytes_engine _engine;
};