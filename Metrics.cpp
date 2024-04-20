#include "Metrics.h"
#include <iostream>

Metrics * Metrics::instance = nullptr;
int Metrics::CURRENT_STORAGE = STORAGE_SSD;

void Metrics::Init()
{
    // the life cycle of the Metrics object is the same as the program
    // destroyed automatically when the program exits
    if (instance == nullptr) {
        instance = new Metrics();
    }
}

bool Metrics::setCurrentStorage(const int device_type)
{
    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        return false;
    }

    CURRENT_STORAGE = device_type;
    return true;
}

void Metrics::read(const int device_type, const u_int64_t num_bytes) // TODO: When to clear space consumed/read?
{
    #ifdef VERBOSEL2
    traceprintf("Accessing storage %d with %lu bytes\n", device_type, num_bytes);
    #endif

    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        throw std::invalid_argument("Invalid device type");
    }

    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];

    // calculate the time spent on data transfer
    metric.dataTransferCost += num_bytes / param.bandwidth;
    metric.numBytes += num_bytes;

    // calculate the fixed latency of the storage system
    metric.accessCost += param.latency;
    metric.numAccesses++;
}

int Metrics::write(const u_int64_t num_bytes)
{

    int device_type = Metrics::CURRENT_STORAGE;
    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];
    if (param.pageSize != num_bytes) {
        std::cerr << "Write size does not match page size" << std::endl; // Use std::cerr instead of cerr
    }
    // calculate the time spent on data transfer
    metric.dataTransferCost += num_bytes / param.bandwidth;
    metric.numBytes += num_bytes;

    // calculate the fixed latency of the storage system
    metric.accessCost += param.latency;
    metric.numAccesses++;

    return device_type;
}

int Metrics::getAvailableStorage()
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    int device_type = Metrics::CURRENT_STORAGE;
    StorageParams & param = instance->params[device_type];
    getAvailableStorage(param.pageSize); // Use the next available storage
    return getAvailableStorage();
}

int Metrics::getAvailableStorage(const u_int64_t num_bytes)
{
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }
    int device_type = Metrics::CURRENT_STORAGE;
    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];
    if (metric.numBytes + num_bytes >= param.capacity) { // Use the next available storage
        if (device_type++ >= NUM_STORAGE_TYPES) throw std::runtime_error("No available storage");
        setCurrentStorage(device_type);
    }
    return getAvailableStorage(num_bytes);
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