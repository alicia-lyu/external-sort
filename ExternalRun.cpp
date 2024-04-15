#include "ExternalRun.h"
#include "utils.h"

ExternalRun::ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize) :
    _runFileName (runFileName), _pageSize (pageSize), _recordSize (recordSize), _pageCount (1)
{
    TRACE (false);
    _runFile = std::ifstream(runFileName, std::ios::binary);
    inMemoryPage = new Buffer(pageSize / recordSize, recordSize);
    _fillPage();
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (false);
    delete inMemoryPage;
    _runFile.close();
} // ExternalRun::~ExternalRun

byte * ExternalRun::next ()
{
    if (_runFile.eof()) {
        // Reaches end of the run
        // traceprintf("End of run file %s\n", _runFileName.c_str());
        return nullptr;
    }
    byte * row = inMemoryPage->next();
    while (row == nullptr) {
        // Reaches end of the page
        u_int32_t readCount = _fillPage();
        if (readCount == 0) return nullptr; 
        // Reaches end of the run after attempting to read a new page
        row = inMemoryPage->next();
    }
    // traceprintf("Read %s from run file %s\n", rowToHexString(row, _recordSize).c_str(), _runFileName.c_str());
    return row;
} // ExternalRun::next

u_int32_t ExternalRun::_fillPage ()
{
    if (_runFile.eof()) {
        throw std::invalid_argument("Reaches end of the run file unexpectedly.");
    }
    if (_runFile.good() == false) {
        throw std::invalid_argument("Error reading from run file.");
    }
    // traceprintf("Filling #%d page from run file %s\n", _pageCount, _runFileName.c_str());
    _runFile.read((char *) inMemoryPage->data(), _pageSize);
    _pageCount++;
    u_int32_t readCount = _runFile.gcount(); // Same scale as _pageSize
    inMemoryPage->batchFillByOverwrite(_runFile.gcount());
    return readCount;
} // ExternalRun::_fillPage