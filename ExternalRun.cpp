#include "ExternalRun.h"
#include "utils.h"

u_int64_t ExternalRun::READ_AHEAD_SIZE = 0; // Initialize with appropriate value
double ExternalRun::READ_AHEAD_THRESHOLD = 0.0; // Initialize with appropriate value

ExternalRun::ExternalRun (const string &runFileName, RowSize recordSize) :
    _readAheadPage (nullptr), _runFileName (runFileName), _runFile (runFileName, std::ios::binary), 
    _recordSize (recordSize),  _produced (0)
{
    TRACE (false);

    // Parse the device types and switch points from the run file name
    // Initialize the storage, nextStorage, and _pageSize
    vector<u_int8_t> deviceTypes;
    vector<u_int64_t> switchPoints;
    std::tie(deviceTypes, switchPoints) = parseDeviceType(runFileName);

    if (switchPoints.size() == 0) switchPoint = 0;
    else if (switchPoints.size() == 1) switchPoint = switchPoints.at(0);
    else throw std::invalid_argument("More than 1 switch points.");
    
    storage = deviceTypes.at(0);
    _pageSize = Metrics::getParams(storage).pageSize;
    if (deviceTypes.size() == 1) nextStorage = storage;
    else if (deviceTypes.size() == 2) {
        nextStorage = deviceTypes.at(1);
        _pageSize = Metrics::getParams(nextStorage).pageSize;
        // INPUT buffer allocated for this run is the largest page size if there are multiple device types
        // OPTIMIZATION: Use the smaller page size when appropriate and allocate the rest to read ahead
        // Concern: Need to retract the space and kill read ahead pages from other runs when switching device
    }
    else throw std::invalid_argument("More than 2 device types.");

    #if defined(VERBOSEL1) || defined(VERBOSEL2)
    if (switchPoints.size() == 1) {
        traceprintf("Run file %s: %zu device types, switch point %llu, page size %d\n", runFileName.c_str(), deviceTypes.size(), switchPoint, _pageSize);
    }
    #endif

    // page size is the largest multiple of record size
    _pageSize = _pageSize / _recordSize * _recordSize;
    
    _currentPage = getBuffer();
    _fillPage(_currentPage);
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (false);
    delete _currentPage;
    Metrics::erase(storage, std::filesystem::file_size(_runFileName) - switchPoint * _recordSize); // OPTIMIZATION: Erase in a smaller granularity at each fill?
    if (_readAheadPage != nullptr) delete _readAheadPage;

    _runFile.close();
    // delete the run file
    if (std::remove(_runFileName.c_str()) != 0) {
        std::cerr << "Error deleting run file " << _runFileName << std::endl;
    }

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
        traceprintf("# %llu row is null, run file: %s\n", _produced, _runFileName.c_str());
        #endif
        bool hasMore = refillCurrentPage();
        row = _currentPage->next();
        Assert((row == nullptr) == !hasMore, __FILE__, __LINE__);
        if (row == nullptr) return nullptr;
    }
    #if defined(VERBOSEL1)
    ++ _produced;
    if (_produced % 10000 == 0) traceprintf("# %llu of %s: %s\n", _produced, _runFileName.c_str(), rowToString(row, _recordSize).c_str());
    #endif
    readAhead(); // Read ahead if meets the criteria
    return row;
} // ExternalRun::next

byte * ExternalRun::peek ()
{
    TRACE (false);
    byte * row = _currentPage->peekNext();
    if (row == nullptr) { // Reaches end of the current page
        bool hasMore = refillCurrentPage();
        row = _currentPage->peekNext();
        Assert((row == nullptr) == !hasMore, __FILE__, __LINE__);
    }
    return row;
} // ExternalRun::peek

u_int32_t ExternalRun::_fillPage (Buffer * page)
{
    u_int32_t readCount = 0;
    TRACE (false);

    if (_runFile.eof()) {
        readCount = 0;
    } else if (_runFile.good() == false) {
        throw std::invalid_argument("Error reading from run file.");
    } else {
        _runFile.read((char *) page->data(), _pageSize);
        readCount = _runFile.gcount(); // Same scale as _pageSize
    }
    
    #if defined(VERBOSEL2)
    traceprintf("Read %d rows from run file %s\n", readCount / _recordSize, _runFileName.c_str());
    #endif
    // Update buffer and metrics with readCount
    if (readCount % _recordSize != 0) throw std::invalid_argument("Read count is not a multiple of record size, from file " + _runFileName + " with read count " + std::to_string(readCount) + " at " + std::to_string(_produced));
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
        u_int64_t nextProducedCount = _produced + _pageSize / _recordSize;
        if (nextProducedCount > switchPoint) {
            traceprintf("# %llu: Switching device before switch point %llu\n", _produced, switchPoint);
            Metrics::erase(storage, _produced * _recordSize);
            // We may leave a small fragment in the next device, left for future OPTIMIZATION
            storage = nextStorage;
        }
    }
    return new Buffer(_pageSize / _recordSize, _recordSize);
} // ExternalRun::getBuffer

bool ExternalRun::refillCurrentPage() 
{
    if (_readAheadPage != nullptr) { // We have a read-ahead page at hand
        delete _currentPage;
        _currentPage = _readAheadPage;
        _readAheadPage = nullptr;
        ExternalRun::READ_AHEAD_SIZE += _pageSize;
    } else { // We need to read a new page (blocking I/O)
        _fillPage(_currentPage);
    }
    return _currentPage->sizeFilled() > 0;
}

void ExternalRun::readAhead()
{
    // With a single thread, it is fundamentally challenging to make read-ahead non-blocking
    // Therefore, we are only mimicking the non-blocking behavior by not counting the read-ahead cost in Metrics
    if ( _readAheadPage == nullptr 
        && ExternalRun::READ_AHEAD_SIZE >= _pageSize 
        && (double) _currentPage->sizeRead() / (_currentPage->recordSize * _currentPage->recordCount) > ExternalRun::READ_AHEAD_THRESHOLD) 
    {
        _readAheadPage = getBuffer();
        u_int32_t readCount = _fillPage(_readAheadPage);
        ExternalRun::READ_AHEAD_SIZE -= readCount;
    }
}