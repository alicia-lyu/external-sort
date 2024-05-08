#pragma once

#include "Buffer.h"
#include "params.h"
#include "Metrics.h"
#include <fstream>
#include <string>

using std::string;
using std::ifstream;

class ExternalRun
{
public:
    static u_int64_t READ_AHEAD_SIZE;
    static double READ_AHEAD_THRESHOLD;
    u_int8_t storage;
    ExternalRun (const string &runFileName, RowSize recordSize);
    ~ExternalRun ();
    byte * next(); // nullptr if the run is empty
    byte * peek(); // nullptr if the run is empty
private:
    Buffer * _currentPage;
    Buffer * _readAheadPage;
    string const _runFileName;
    ifstream _runFile;
    u_int32_t _pageSize; // max. 500 KB = 2^19, changes when switching between SSD and HDD
    RowSize const _recordSize;
    u_int64_t _produced;
    u_int64_t switchPoint;
    u_int8_t nextStorage;
    u_int32_t _fillPage(Buffer * page);
    Buffer * getBuffer();
    bool refillCurrentPage(); // Either by filling the current page or switching to the read-ahead page
    void readAhead();
};