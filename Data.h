#pragma once

#include "defs.h"
#include <memory>
#include <random>

typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: 

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, byte>;
using std::random_device;

class Buffer // an in-memory buffer
{
public:
    u_int16_t count; // max: 100 MB / 20 B = 5 * 10^3 = 2^13
    RowSize size;
    Buffer (u_int16_t count, RowSize size);
    ~Buffer ();
    byte * fillRandomly();
    byte * copy(byte const * source);
private:
    byte * _filled;
    byte * _rows;
    random_bytes_engine _engine;
    random_device _device;
};