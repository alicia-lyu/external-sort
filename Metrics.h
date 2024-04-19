# pragma once

#include "params.h"
#include "Buffer.h"
#include <tuple>
using std::tuple;

/*
StorageMetrics: the metrics of a storage system (SSD / HDD)
    dataTransferCost: the time spent on data transfer
    accessCost: the fixed latency of the storage system,
                equal to (number of access) * (latency per access)
*/
struct StorageMetrics {
    double dataTransferCost;
    double accessCost;
    u_int64_t numBytes;
    u_int64_t numAccesses;

    StorageMetrics(double _dataTransferCost = 0.0, double _accessCost = 0.0,
        u_int64_t _numBytes = 0, u_int64_t _numAccesses = 0) : 
        dataTransferCost(_dataTransferCost), accessCost(_accessCost),
        numBytes(_numBytes), numAccesses(_numAccesses) {}
};

/*
StorageParams: the parameters of a storage system (SSD / HDD)
    latency: the latency of each access, in seconds
    bandwidth: the bandwidth of the storage system, in bytes per second
*/
struct StorageParams {
    double latency;
    double bandwidth;

    StorageParams(double _latency = 0.0, double _bandwidth = 0.0) : 
        latency(_latency), bandwidth(_bandwidth) {}
};

class Metrics
{
public:
    static bool accessStorage(const int device_type, const u_int64_t num_bytes);
    static StorageMetrics getMetrics(const int device_type);
    static void Init();
    Metrics();
    static int CURRENT_STORAGE;
    static bool setCurrentStorage(const int device_type);

private:
    StorageMetrics metrics[NUM_STORAGE_TYPES];
    StorageParams params[NUM_STORAGE_TYPES];

    static Metrics * instance;
};