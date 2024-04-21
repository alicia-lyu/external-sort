#pragma once

#include "defs.h"
#include <memory>
#include <random>

typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: 

using std::random_device;
using std::default_random_engine;
using std::uniform_int_distribution;

class Buffer // an in-memory buffer
{
public:
    u_int16_t const recordCount; // max: 100 MB / 20 B = 5 * 10^3 = 2^13
    RowSize const recordSize;
    u_int64_t const pageSize;
    Buffer (u_int16_t recordCount, RowSize recordSize);
    ~Buffer ();
    byte * fillRandomly(); // Fill the first bytes not filled yet with random bytes. Return the pointer to the filled bytes.
    byte * copy(byte const * source); // Copy the source to the first bytes not filled yet. Return the pointer to the filled bytes.
    byte * next(); // Read the next row after toBeRead. If toBeRead is at the end of the buffer, return nullptr, and set toBeRead to the beginning of the buffer.
    byte * batchFillByOverwrite(u_int64_t toBeFilled); // Returning the beginning of the buffer, and set toBeFilled to the last byte filled. Overwrite existing buffer. Expect to have the designated memory space filled immediately after being called.
    void reset() { toBeRead = _rows; toBeFilled = _rows; };
    byte * data() { return _rows; };
    u_int64_t sizeFilled() { return toBeFilled - _rows; };
    u_int64_t sizeRead() { return toBeRead - _rows; };
    // alpha numeric characters
    static byte toAlphaNumeric(const byte randomByte);
private:
    byte * toBeFilled;
    byte * toBeRead;
    byte * _rows;
    default_random_engine _engine;
    random_device _device;
    uniform_int_distribution<byte> _distribution;
    byte getRandomAlphaNumeric();
    static const int RANDOM_BYTE_UPPER_BOUND = (26+26+10); // 26 upper case, 26 lower case, 10 digits
};