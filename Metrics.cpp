#include "Metrics.h"
#include <iostream>

Metrics * Metrics::instance = nullptr;

void Metrics::Init()
{
    // the life cycle of the Metrics object is the same as the program
    // destroyed automatically when the program exits
    if (instance == nullptr) {
        instance = new Metrics();
    }
}

void Metrics::read(const int device_type, const u_int64_t num_bytes, bool readAhead)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }
    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        throw std::invalid_argument("Invalid device type");
    }
    if (num_bytes == 0) {
        return;
    }
    #ifdef VERBOSEL2
    traceprintf("Accessing storage %d with %lu bytes\n", device_type, num_bytes);
    #endif

    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];

    double latency = 0.0;

    // calculate the time spent on data transfer and fixed latency if not read ahead
    if (!readAhead) {
        metric.dataTransferCost += num_bytes / param.bandwidth;
        metric.accessCost += param.latency;
        latency = num_bytes / param.bandwidth + param.latency;
    }

    metric.numBytesRead += num_bytes;
    metric.numAccessesRead++;

    #ifdef PRODUCTION
    Trace::TraceAccess(ACCESS_READ, device_type, latency, num_bytes);
    #endif
}

int Metrics::write(const int device_type, const u_int64_t num_bytes)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }
    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];
    if (metric.storageUsed + num_bytes > param.capacity) { 
        throw std::runtime_error("No available storage on device" + std::to_string(device_type) + " for " + std::to_string(num_bytes) + " bytes." + " Current storage used: " + std::to_string(metric.storageUsed) + " bytes." + " Capacity: " + std::to_string(param.capacity) + " bytes.");
    }
    if (param.pageSize != num_bytes) {
        // std::cerr << "Warning: Write size " << num_bytes << " does not match page size " << param.pageSize << ". This may be expected only when you are flushing the last fragment of a file." << std::endl; // Use std::cerr instead of cerr
    }
    // calculate the time spent on data transfer
    metric.dataTransferCost += num_bytes / param.bandwidth;
    metric.numBytesWritten += num_bytes;

    // calculate the fixed latency of the storage system
    metric.accessCost += param.latency;
    metric.numAccessesWritten++;

    metric.storageUsed += num_bytes;

    #ifdef PRODUCTION
    // output to trace
    double latency = num_bytes / param.bandwidth + param.latency;
    Trace::TraceAccess(ACCESS_WRITE, device_type, latency, num_bytes);
    #endif

    return device_type;
}

void Metrics::erase(const int device_type, const u_int64_t num_bytes) {
    // To keep available storage on each device updated
    // In a real system, we don't need extra IO for erase, by just overwriting the data.
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }
    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        throw std::invalid_argument("Invalid device type");
    }
    #if defined(VERBOSEL2)
    traceprintf("Erasing %llu bytes from storage %d\n", num_bytes, device_type);
    #endif
    StorageMetrics & metric = instance->metrics[device_type];
    metric.storageUsed -= num_bytes;
}

int Metrics::getAvailableStorage()
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    for (int device_type = 0; device_type < NUM_STORAGE_TYPES; device_type++) {
        StorageParams & param = instance->params[device_type];
        StorageMetrics & metric = instance->metrics[device_type];
        if (metric.storageUsed + param.pageSize <= param.capacity) {
            return device_type;
        }
    }
    throw std::runtime_error("No available storage");
}

int Metrics::getAvailableStorage(const u_int64_t num_bytes)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }
    for (int device_type = 0; device_type < NUM_STORAGE_TYPES; device_type++) {
        StorageParams & param = instance->params[device_type];
        StorageMetrics & metric = instance->metrics[device_type];
        if (metric.storageUsed + num_bytes <= param.capacity) {
            return device_type;
        } else if (metric.storageUsed + param.pageSize <= param.capacity) {
            std::cerr << "Warning: Storage " << device_type << " is skipped, despite having at least space of one page size available." << std::endl;
        }
    }
    throw std::runtime_error("No available storage for " + std::to_string(num_bytes) + " bytes");
}

StorageMetrics Metrics::getMetrics(const int device_type)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        throw std::invalid_argument("Invalid device type");
    }

    return instance->metrics[device_type];
}

StorageParams Metrics::getParams(const int device_type)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        throw std::invalid_argument("Invalid device type");
    }

    return instance->params[device_type];
}

Metrics::Metrics()
{
    params[STORAGE_SSD] = StorageParams(SSD_LATENCY, SSD_BANDWIDTH, SSD_SIZE, SSD_PAGE_SIZE);
    params[STORAGE_HDD] = StorageParams(HDD_LATENCY, HDD_BANDWIDTH, INT64_MAX, HDD_PAGE_SIZE); // unlimited capacity

    metrics[STORAGE_SSD] = StorageMetrics();
    metrics[STORAGE_HDD] = StorageMetrics();
}