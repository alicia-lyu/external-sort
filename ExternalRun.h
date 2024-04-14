#include "Buffer.h"
#include <fstream>


class ExternalRun
{
public:
    Buffer * inMemoryPage;
    ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize);
    ~ExternalRun ();
    byte * next();
private:
    std::string _runFileName;
    std::ifstream _runFile;
    u_int32_t _read;
    u_int32_t _pageSize; // max. 500 KB = 2^19
    RowSize _recordSize;
    u_int32_t _pageCount; // max. 120 GB / 50 KB = 2^21
    void _fillPage();
};