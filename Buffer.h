#pragma once

#include "defs.h"
#include <memory>
#include <random>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <algorithm>


using std::default_random_engine;
using std::uniform_int_distribution;
using std::ifstream;
using std::string;

// class Buffer
// provides a buffer to store records
// required function:
//  - next: virtual function; returns the next record, in byte *
//  - Buffer(recordCount, recordSize): constructor
//  - Buffer(recordCount, recordSize, inputFile): constructor
class Buffer
{
public:
    u_int32_t const recordCount; // max: 100 MB / 20 B = 2 ^ 25
    RowSize const recordSize;
    u_int64_t const pageSize;
    Buffer (u_int32_t recordCount, RowSize recordSize);
    virtual ~Buffer ();
    byte * data() { return _rows; };
    u_int64_t sizeFilled() { return toBeFilled - _rows; };
    u_int64_t sizeRead() { return toBeRead - _rows; };
    byte * copy(byte const * source); // Copy the source to the first bytes not filled yet. Return the pointer to the filled bytes.
    byte * batchFillByOverwrite(u_int64_t toBeFilled); // Returning the beginning of the buffer, and set toBeFilled to the last byte filled. Overwrite existing buffer. Expect to have the designated memory space filled immediately after being called.
    byte * peekNext();
    virtual byte * next(); // Read the next row after toBeRead. If toBeRead is at the end of the buffer, return nullptr, and set toBeRead to the beginning of the buffer.
protected:
    byte * _rows;
    byte * _rowsEnd;
    byte * toBeRead;
    byte * toBeFilled;
};


// RandomBuffer: derived from Buffer
// generates random records
// overrides the next function to generate random records
class RandomBuffer : public Buffer
{
public:
    RandomBuffer (u_int32_t recordCount, RowSize recordSize);
    ~RandomBuffer ();
    byte * fillRandomly(); // Fill the first bytes not filled yet with random bytes. Return the pointer to the filled bytes.
    // alpha numeric characters
    static byte toAlphaNumeric(const byte randomByte);
    byte * next() override;
private:
    default_random_engine _engine;
    uniform_int_distribution<byte> _distribution;
    byte getRandomAlphaNumeric();
    static const int RANDOM_BYTE_UPPER_BOUND = (26+26+10); // 26 upper case, 26 lower case, 10 digits

};


// InFileBuffer: derived from Buffer
// reads records from a file
// overrides the next function to read records from a file
class InFileBuffer : public Buffer
{
public:
    InFileBuffer (u_int32_t recordCount, RowSize recordSize, const string & inputPath);
    ~InFileBuffer ();
    byte * next() override;
private:
    ifstream _inputFile;
    bool isAlphaNumeric(byte c);
    bool readRecordFromFile();
};