#include "ExternalRun.h"
#include "utils.h"

ExternalRun::ExternalRun (std::string runFileName, RowSize recordSize, u_int64_t & readAheadSize) :
    storage (parseDeviceType(runFileName)), _readAheadSize (readAheadSize), 
    _readAheadThreshold (std::max(1.0 - (double) readAheadSize / MEMORY_SIZE, 0.5)),
    _readAheadPage (nullptr), _runFileName (runFileName), _runFile (runFileName, std::ios::binary), 
    _pageSize (Metrics::getParams(storage).pageSize), _recordSize (recordSize),  _produced (0)
{
    TRACE (false);
    #ifdef VERBOSEL1
    traceprintf("Run file %s, device type %d, page size %d, read ahead size %llu above %f\n", 
        runFileName.c_str(), storage, _pageSize, readAheadSize, _readAheadThreshold);
    #endif
    _currentPage = new Buffer(_pageSize / recordSize, recordSize);
    _fillPage(_currentPage);
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (false);
    delete _currentPage;
    if (_readAheadPage != nullptr) delete _readAheadPage;
    _runFile.close();
    #if defined(VERBOSEL1) || defined(VERBOSEL2)
    traceprintf("Produced %llu rows from run file %s\n", _produced, _runFileName.c_str());
    #endif
} // ExternalRun::~ExternalRun

byte * ExternalRun::next ()
{
    TRACE (false);
    if (_runFile.eof()) {
        return nullptr;
    }
    byte * row = _currentPage->next();
    while (row == nullptr) {
        // Reaches end of the current page
        if (_readAheadPage != nullptr) { // We have a read-ahead page at hand
            delete _currentPage;
            _currentPage = _readAheadPage;
            _readAheadPage = nullptr;
            _readAheadSize += _pageSize;
            #if defined(VERBOSEL1)
            traceprintf("Run %s switched to read-ahead page at %llu remaining read ahead buffer size: %llu\n", _runFileName.c_str(), _produced, _readAheadSize);
            #endif
        } else { // We need to read a new page (blocking I/O)
            u_int32_t readCount = _fillPage(_currentPage);
            if (readCount == 0) return nullptr; // Reaches end of the run after attempting to read a new page
        }
        row = _currentPage->next();
    }
    ++ _produced;
    // Read-ahead logic TODO: make it non-blocking
    u_int16_t recordPerPage = _pageSize / _recordSize; // max. 500 KB / 20 B = 25000 = 2^14
    if (_produced % (recordPerPage / 10) == 0 && _readAheadPage == nullptr && _readAheadSize >= _pageSize) {
        if ((double) _currentPage->sizeRead() / (_currentPage->recordSize * _currentPage->recordCount) > _readAheadThreshold) {
            _readAheadPage = new Buffer(_pageSize / _recordSize, _recordSize);
            _fillPage(_readAheadPage);
            if (_readAheadSize > _pageSize) _readAheadSize -= _pageSize;
            else throw std::runtime_error("Contention on decreasing read ahead size.");
        }
    }
    return row;
} // ExternalRun::next

u_int32_t ExternalRun::_fillPage (Buffer * page) // metrics
{
    TRACE (false);
    if (_runFile.eof()) throw std::invalid_argument("Reaches end of the run file unexpectedly.");
    if (_runFile.good() == false) throw std::invalid_argument("Error reading from run file.");
    #if defined(VERBOSEL1)
    traceprintf("Filling %s page from run file %s, remaining read ahead size %llu\n", page == _currentPage ? "current" : "read-ahead", _runFileName.c_str(), _readAheadSize);
    #endif
    _runFile.read((char *) page->data(), _pageSize); // TODO: switch device
    u_int32_t readCount = _runFile.gcount(); // Same scale as _pageSize
    page->batchFillByOverwrite(readCount);
    Metrics::read(storage, readCount, page == _readAheadPage);
    return readCount;
} // ExternalRun::_fillPage