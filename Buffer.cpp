#include "Buffer.h"

Buffer::Buffer (u_int16_t count, RowSize size):
    count (count), size (size), _engine(_device()), _device(),_distribution(0, RANDOM_BYTE_UPPER_BOUND - 1)
{
    TRACE (false);
    _rows = (byte *) malloc(size * count * sizeof(byte));
    _read = _rows;
    _filled = _rows;
}

Buffer::~Buffer ()
{
    TRACE (false);
    free(_rows);
}

/*
Generate random bytes and convert them to alphanumeric characters (A-Z, a-z, 0-9)
randomByte: a random byte (0-61)
0-9 -> '0'-'9' (48-57)
10-35 -> 'A'-'Z' (65-90)
36-61 -> 'a'-'z' (97-122)
*/
byte Buffer::toAlphaNumeric (const byte randomByte) {
    const byte numericOffset = 48;
    const byte upperCaseOffset = 65;
    const byte lowerCaseOffset = 97;
    if (randomByte < 10) {
        return randomByte + numericOffset;
    } else if (randomByte < 36) {
        return randomByte - 10 + upperCaseOffset;
    } else {
        return randomByte - 36 + lowerCaseOffset;
    }
}

byte Buffer::getRandomAlphaNumeric ()
{
    byte randomByte = _distribution(_engine);
    return toAlphaNumeric(randomByte);
}

byte * Buffer::fillRandomly ()
{
    if (_filled >= _rows + size * count) {
        // traceprintf("Buffer is full, overwriting the first row\n");
        // CODE IMPROVEMENT: uniformize the behavior of overwriting the first row (return nullptr to indicate the end of the buffer)
        _filled = _rows;
    }
    std::generate(_filled, _filled + size, [this](){ return getRandomAlphaNumeric(); });
    _filled += size;
    return _filled - size;
}

byte * Buffer::copy (byte const * source)
{
    if (_filled >= _rows + size * count) {
        // traceprintf("Buffer is full.\n");
        _filled = _rows;
        return nullptr;
    }
    std::copy(source, source + size, _filled);
    _filled += size;
    return _filled - size;
}

byte * Buffer::next ()
{
    if (_read >= _rows + size * count || _read > _filled) { 
        // If reaches the end of the buffer, return nullptr and reset _read to the beginning of the buffer.
        _read = _rows;
        return nullptr;
    }
    _read += size;
    return _read - size;
}

byte * Buffer::batchFillByOverwrite (u_int64_t toBeFilled)
{
    if (toBeFilled > size * count) {
        throw std::runtime_error("Buffer overflow");
    } else if (toBeFilled < size * count) {
        traceprintf("Buffer under-filled %llu / %d.\n", toBeFilled, size * count);
    }
    _filled = _rows + toBeFilled;
    _read = _rows;
    return _rows;
}