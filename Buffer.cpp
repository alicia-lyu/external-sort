#include "Buffer.h"

// Base class: Buffer
Buffer::Buffer(u_int32_t recordCount, RowSize recordSize):
    recordCount(recordCount), recordSize(recordSize), pageSize(recordCount * recordSize)
{
    TRACE(false);
    _rows = (byte *) malloc(recordSize * recordCount * sizeof(byte));
    if (_rows == nullptr) {
        throw std::runtime_error("Buffer allocation failed");
    }
    toBeRead = _rows;
    toBeFilled = _rows;
    _rowsEnd = _rows + recordSize * recordCount;
    #if defined(VERBOSEL2)
    traceprintf("Buffer created with %d records of size %d, page size %llu.\n", recordCount, recordSize, pageSize);
    #endif
}

Buffer::~Buffer ()
{
    TRACE (false);
    free(_rows);
}

byte * Buffer::copy (byte const * source)
{
    if (toBeFilled >= _rowsEnd) {
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
    TRACE (false);
    if (toBeRead >= _rowsEnd || toBeRead >= toBeFilled) { 
        toBeRead = _rows;
        toBeFilled = _rows; // Filled rows are all read
        return nullptr;
    }
    byte * read = toBeRead;
    toBeRead += recordSize;
    return read;
}

byte * Buffer::peekNext ()
{
    TRACE (false);
    // Only update toBeRead and toBeFilled when the buffer is exhausted
    // This means calling peekNext multiple times will return the same record
    // except when the buffer is exhausted
    if (toBeRead >= _rowsEnd || toBeRead >= toBeFilled) {
        toBeRead = _rows;
        toBeFilled = _rows;
        return nullptr;
    }
    return toBeRead;
}

byte * Buffer::batchFillByOverwrite (u_int64_t sizeToBeFilled)
{
    if (sizeToBeFilled > recordSize * recordCount) {
        throw std::runtime_error("Buffer overflow");
    } else if (sizeToBeFilled < recordSize * recordCount && sizeToBeFilled > 0) {
        #if defined(VERBOSEL1)
        traceprintf("Buffer under-filled %llu / %d.\n", sizeToBeFilled, recordSize * recordCount);
        #endif
    }
    if (toBeFilled > _rows) {
        std::cerr << "Warning: Existing content overwritten by batchFillByOverwrite" << std::endl;
    }
    toBeFilled = _rows + sizeToBeFilled;
    toBeRead = _rows;
    return _rows;
}



// Derived class: RandomBuffer
RandomBuffer::RandomBuffer (u_int32_t recordCount, RowSize recordSize):
    Buffer(recordCount, recordSize),
    _engine(std::chrono::system_clock::now().time_since_epoch().count()),
    _distribution(0, RANDOM_BYTE_UPPER_BOUND - 1)
{
    TRACE (false);
    // nothing to do here right now
}

RandomBuffer::~RandomBuffer ()
{
    TRACE (false);
}

/*
Generate random bytes and convert them to alphanumeric characters (A-Z, a-z, 0-9)
randomByte: a random byte (0-61)
0-9 -> '0'-'9' (48-57)
10-35 -> 'A'-'Z' (65-90)
36-61 -> 'a'-'z' (97-122)
*/
byte RandomBuffer::toAlphaNumeric (const byte randomByte) {
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

byte RandomBuffer::getRandomAlphaNumeric ()
{
    byte randomByte = _distribution(_engine);
    return toAlphaNumeric(randomByte);
}

byte * RandomBuffer::fillRandomly ()
{
    if (toBeFilled >= _rowsEnd) {
        toBeFilled = _rows;
        #if defined(VERBOSEL1) || defined(VERBOSEL2)
        traceprintf("Buffer cleared for refill.\n");
        #endif
        return nullptr;
    }
    std::generate(toBeFilled, toBeFilled + recordSize, [this](){ return getRandomAlphaNumeric(); });
    byte * filled = toBeFilled;
    toBeFilled += recordSize;
    return filled;
}

byte * RandomBuffer::next ()
{
    TRACE (false);
    byte * nextRecord = fillRandomly();
    // toBeRead is always the same as toBeFilled, since 
    // we generate random records on the fly
    toBeRead = toBeFilled;
    return nextRecord;
}


// Derived class: InFileBuffer
InFileBuffer::InFileBuffer (u_int32_t recordCount, RowSize recordSize, const string & inputPath):
    Buffer(recordCount, recordSize)
{
    TRACE (false);
    _inputFile = ifstream (inputPath, std::ios::binary);
    if (!_inputFile.is_open()) {
        throw std::runtime_error("Input file not found");
    }
}

InFileBuffer::~InFileBuffer ()
{
    TRACE (false);
    _inputFile.close();
}

bool InFileBuffer::isAlphaNumeric (byte c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool InFileBuffer::readRecordFromFile ()
{
    u_int16_t numToRead = recordSize;
    char c;

    while (numToRead > 0) {
        if (_inputFile.eof()) {
            return false;
        }
        _inputFile.read(&c, sizeof(char));
        if (isAlphaNumeric(c)) {
            *toBeFilled = (byte) c;
            ++toBeFilled;
            --numToRead;
        }
    }

    return true;
}

byte * InFileBuffer::next ()
{
    TRACE (false);
    if (toBeRead >= _rowsEnd || toBeRead >= toBeFilled) {
        // has exhausted all records previously read, need to read more
        toBeRead = toBeFilled = _rows;

        // get one more record
        for (u_int16_t i = 0; i < recordCount; ++i) {
            auto res = readRecordFromFile();
            if (!res) {
                break;
            }
        }
    }

    // still no data, return nullptr
    if (toBeRead >= _rowsEnd || toBeRead >= toBeFilled) {
        return nullptr;
    }

    // return the next record
    byte * read = toBeRead;
    toBeRead += recordSize;
    return read;
}