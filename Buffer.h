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
    byte * fillRandomly(); // Fill the first bytes not filled yet with random bytes. Return the pointer to the filled bytes.
    byte * copy(byte const * source); // Copy the source to the first bytes not filled yet. Return the pointer to the filled bytes.
    byte * next(); // Read the next row after _read. If _read is at the end of the buffer, return nullptr, and set _read to the beginning of the buffer.
    byte * batchFillByOverwrite(u_int64_t toBeFilled); // Returning the beginning of the buffer, and set _filled to the last byte filled. Overwrite existing buffer. Expect to have the designated memory space filled immediately after being called.
    void reset() { _read = _rows; _filled = _rows; };
    byte * data() { return _rows; };
    u_int64_t sizeFilled() { return _filled - _rows; };
private:
    byte * _filled;
    byte * _read;
    byte * _rows;
    random_bytes_engine _engine;
    random_device _device;
};