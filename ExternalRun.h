#include "Buffer.h"
#include "params.h"
#include <fstream>

class ExternalRun
{
public:
    ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize, u_int16_t & readAheadBuffers, double const readAheadThreshold);
    ~ExternalRun ();
    byte * next();
private:
    u_int16_t & _readAheadBuffers;
    double const _readAheadThreshold;
    Buffer * _currentPage;
    Buffer * _readAheadPage;
    std::string _runFileName;
    std::ifstream _runFile;
    u_int32_t _pageSize; // max. 500 KB = 2^19
    RowSize _recordSize;
    u_int64_t _produced;
    u_int32_t _fillPage(Buffer * page);
};