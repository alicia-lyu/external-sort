#include "Buffer.h"

Buffer::Buffer (u_int16_t recordCount, RowSize recordSize):
    recordCount (recordCount), recordSize (recordSize), pageSize (recordCount * recordSize),
    _engine(_device()), _device(),_distribution(0, RANDOM_BYTE_UPPER_BOUND - 1)
{
    TRACE (false);
    _rows = (byte *) malloc(recordSize * recordCount * sizeof(byte));
    toBeRead = _rows;
    toBeFilled = _rows;
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
    if (toBeFilled >= _rows + recordSize * recordCount) {
        toBeFilled = _rows;
        return nullptr;
    }
    std::generate(toBeFilled, toBeFilled + recordSize, [this](){ return getRandomAlphaNumeric(); });
    byte * filled = toBeFilled;
    toBeFilled += recordSize;
    return filled;
}

byte * Buffer::copy (byte const * source)
{
    if (toBeFilled >= _rows + recordSize * recordCount) {
        toBeFilled = _rows;
        return nullptr;
    }
    std::copy(source, source + recordSize, toBeFilled);
    byte * filled = toBeFilled;
    toBeFilled += recordSize;
    return filled;
}

byte * Buffer::next ()
{
    if (toBeRead >= _rows + recordSize * recordCount || toBeRead >= toBeFilled) { 
        toBeRead = _rows;
        return nullptr;
    }
    byte * read = toBeRead;
    toBeRead += recordSize;
    return read;
}

byte * Buffer::batchFillByOverwrite (u_int64_t sizeToBeFilled)
{
    if (sizeToBeFilled > recordSize * recordCount) {
        throw std::runtime_error("Buffer overflow");
    } else if (sizeToBeFilled < recordSize * recordCount) {
        #if defined(VERBOSEL2)
        traceprintf("Buffer under-filled %llu / %d.\n", toBeFilled, recordSize * recordCount);
        #endif
    }
    toBeFilled = _rows + sizeToBeFilled;
    toBeRead = _rows;
    return _rows;
}