#include "ExternalRun.h"
#include "utils.h"

ExternalRun::ExternalRun (std::string runFileName, RowSize recordSize, u_int64_t & readAheadSize) :
    _readAheadSize (readAheadSize), 
    _readAheadThreshold (std::max(1.0 - (double) readAheadSize / MEMORY_SIZE, 0.5)),
    _readAheadPage (nullptr), _runFileName (runFileName), _runFile (runFileName, std::ios::binary), 
    _recordSize (recordSize),  _produced (0)
{
    TRACE (false);
    #ifdef VERBOSEL2
    traceprintf("Run file %s, device type %d, page size %d, read ahead size %llu above %f\n", 
        runFileName.c_str(), storage, _pageSize, _readAheadSize, _readAheadThreshold);
    #endif
    vector<u_int8_t> deviceTypes;
    vector<u_int64_t> switchPoints;
    std::tie(deviceTypes, switchPoints) = parseDeviceType(runFileName);

    if (switchPoints.size() == 0) switchPoint = 0;
    else if (switchPoints.size() == 1) switchPoint = switchPoints.at(0);
    else throw std::invalid_argument("More than 1 switch points.");
    
    storage = deviceTypes.at(0);
    _pageSize = Metrics::getParams(storage).pageSize;
    if (deviceTypes.size() == 1) nextStorage = storage;
    else if (deviceTypes.size() == 2) nextStorage = deviceTypes.at(1);
    else throw std::invalid_argument("More than 2 device types.");

    #if defined(VERBOSEL2)
    traceprintf("Switch point %llu, storage %d, next storage %d\n", switchPoint, storage, nextStorage);
    #endif
    
    _currentPage = getBuffer();
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
    byte * row = _currentPage->next();
    if (row == nullptr) { // Reaches end of the current page
        #if defined(VERBOSEL2)
        traceprintf("# %llu row is null: %s buffer read %d\n", _produced, _runFileName.c_str(), _currentPage->sizeRead() / _recordSize);
        #endif
        if (_readAheadPage != nullptr) { // We have a read-ahead page at hand
            delete _currentPage;
            _currentPage = _readAheadPage;
            _readAheadPage = nullptr;
            _readAheadSize += _pageSize;
        } else { // We need to read a new page (blocking I/O)
            u_int32_t readCount = _fillPage(_currentPage);
        }
        row = _currentPage->next();
        if (row == nullptr) { // Still null, we have reached the end of the run
            Metrics::erase(storage, std::filesystem::file_size(_runFileName) - switchPoint * _recordSize);
            // OPTIMIZATION: Erase in a smaller granularity at each fill?
            return nullptr;
        }
    } 
    // COMMENT: With a single thread, it is fundamentally challenging to make read-ahead non-blocking
    // Therefore, we are only mimicking the non-blocking behavior by not counting the read-ahead cost in Metrics
    u_int16_t recordPerPage = _pageSize / _recordSize; // max. 500 KB / 20 B = 25000 = 2^14
    if (_produced % (recordPerPage / 10) == 0 
        && _readAheadPage == nullptr 
        && _readAheadSize >= _pageSize 
        && (double) _currentPage->sizeRead() / (_currentPage->recordSize * _currentPage->recordCount) > _readAheadThreshold) 
    {
        _readAheadPage = getBuffer();
        u_int32_t readCount = _fillPage(_readAheadPage);
        _readAheadSize -= readCount;
    }
    #if defined(VERBOSEL2)
    if (_produced % 1000 == 0) traceprintf("# %llu of %s: %s\n", _produced, _runFileName.c_str(), rowToString(row, _recordSize).c_str());
    #endif
    ++ _produced;
    return row;
} // ExternalRun::next

u_int32_t ExternalRun::_fillPage (Buffer * page)
{
    TRACE (false);
    if (_runFile.eof()) return 0;
    if (_runFile.good() == false) throw std::invalid_argument("Error reading from run file.");
    _runFile.read((char *) page->data(), _pageSize);
    auto readCount = _runFile.gcount(); // Same scale as _pageSize
    #if defined(VERBOSEL2)
    traceprintf("Read %d rows from run file %s\n", readCount / _recordSize, _runFileName.c_str());
    #endif
    page->batchFillByOverwrite(readCount);
    Metrics::read(storage, readCount, page == _readAheadPage);
    return readCount;
} // ExternalRun::_fillPage

Buffer * ExternalRun::getBuffer ()
{
    TRACE (false);
    if (nextStorage != storage) // If they are the same, we've reached the last device
    {
        // If the switch point is estimated to land in the next page, we need to switch to the next device
        // By "estimated", we are not considering the rows not produced yet in the current page; this only happens 
        // when we are reading ahead, i.e. when we are not so sensitive to suboptimal choice of page size
        u_int32_t nextPageSize = Metrics::getParams(nextStorage).pageSize;
        u_int64_t nextProducedCount = _produced + nextPageSize / _recordSize;
        if (nextProducedCount > switchPoint) {
            traceprintf("# %llu: Switching device before switch point %llu, page size %d -> %d\n", _produced, switchPoint, _pageSize, nextPageSize);
            Metrics::erase(storage, _produced * _recordSize);
            // We may leave a small fragment in the next device, left for future optimization
            storage = nextStorage;
            _pageSize = nextPageSize;
        }
    }
    return new Buffer(_pageSize / _recordSize, _recordSize);
} // ExternalRun::getBuffer