#include "Buffer.h"
#include "params.h"
#include "Metrics.h"
#include <fstream>

class ExternalRun
{
public:
    u_int8_t storage;
    ExternalRun (std::string runFileName, RowSize recordSize, u_int64_t & readAheadSize);
    ~ExternalRun ();
    byte * next();
private:
    u_int64_t _readAheadSize;
    double const _readAheadThreshold;
    Buffer * _currentPage;
    Buffer * _readAheadPage;
    std::string const _runFileName;
    std::ifstream _runFile;
    u_int32_t _pageSize; // max. 500 KB = 2^19, changes when switching between SSD and HDD
    RowSize const _recordSize;
    u_int64_t _produced;
    u_int64_t switchPoint;
    u_int8_t nextStorage;
    u_int32_t _fillPage(Buffer * page);
    Buffer * getBuffer();
};