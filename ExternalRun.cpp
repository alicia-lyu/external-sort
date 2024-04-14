#include "ExternalRun.h"
#include "utils.h"

ExternalRun::ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize) :
    _runFileName (runFileName), _read (0), _pageSize (pageSize), _recordSize (recordSize), _pageCount (1)
{
    TRACE (true);
    _runFile = std::ifstream(runFileName, std::ios::binary);
    if (!_runFile) {
        throw std::invalid_argument("Run file does not exist");
    }
    inMemoryPage = new Buffer(pageSize / recordSize, recordSize);
    _fillPage();
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (true);
    delete inMemoryPage;
    _runFile.close();
} // ExternalRun::~ExternalRun

byte * ExternalRun::next ()
{
    traceprintf("Reading from run file %s\n", _runFileName.c_str());
    if (_runFile.eof()) {
        // Reaches end of the run
        traceprintf("End of run file %s\n", _runFileName.c_str());
        return nullptr;
    }
    byte * row = inMemoryPage->next();
    while (row == nullptr) {
        // Reaches end of the page
        _fillPage();
        row = inMemoryPage->next();
    }
    traceprintf("Read %s from run file %s\n", rowToHexString(row, _recordSize).c_str(), _runFileName.c_str());
    return row;
} // ExternalRun::next

void ExternalRun::_fillPage ()
{
    if (_runFile.eof()) {
        throw std::invalid_argument("Reaches end of the run file unexpectedly.");
    }
    traceprintf("Filling #%d page from run file %s\n", _pageCount, _runFileName.c_str());
    _runFile.read((char *) inMemoryPage->data(), _pageSize);
    _pageCount++;
    inMemoryPage->batchFillByOverwrite(_runFile.gcount());
} // ExternalRun::_fillPage