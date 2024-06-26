# pragma once

#include "params.h"
#include "Buffer.h"
#include <tuple>
using std::tuple;
using std::string;
using std::to_string;

/*
StorageMetrics: the metrics of a storage system (SSD / HDD)
    dataTransferCost: the time spent on data transfer
    accessCost: the fixed latency of the storage system,
                equal to (number of access) * (latency per access)
*/
struct StorageMetrics {
    double dataTransferCost;
    double accessCost;
    u_int64_t numBytesRead;
    u_int64_t numBytesWritten;
    u_int64_t numAccessesRead;
    u_int64_t numAccessesWritten;
    u_int64_t storageUsed;

    StorageMetrics() : 
        dataTransferCost(0.0), accessCost(0.0),
        numBytesRead (0), numBytesWritten (0),
        numAccessesRead (0), numAccessesWritten (0),
        storageUsed (0) {}
};

/*
StorageParams: the parameters of a storage system (SSD / HDD)
    latency: the latency of each access, in seconds
    bandwidth: the bandwidth of the storage system, in bytes per second
*/
struct StorageParams {
    double latency;
    double bandwidth;
    u_int64_t capacity;
    u_int32_t pageSize; // max. 500 KB = 2 ^ 19

    StorageParams(double _latency = 0.0, double _bandwidth = 0.0, u_int64_t _capacity = 0, u_int32_t _pageSize = 0) : 
        latency(_latency), bandwidth(_bandwidth), capacity (_capacity), pageSize (_pageSize) {}
};

class Metrics
{
public:
    static void read(const int device_type, const u_int64_t num_bytes, bool readAhead); // expect filename to contain the device type
    static int write(const int device_type, const u_int64_t num_bytes);
    static void erase(const int device_type, const u_int64_t num_bytes);
    static int getAvailableStorage(); // return the device type that has enough space for predefined page size
    static int getAvailableStorage(const u_int64_t num_bytes); // return the device type that has enough space for a given number of bytes
    static StorageMetrics getMetrics(const int device_type);
    static StorageParams getParams(const int device_type);
    static void Init();
    Metrics();

private:
    StorageMetrics metrics[NUM_STORAGE_TYPES];
    StorageParams params[NUM_STORAGE_TYPES];
    static Metrics * instance;
};