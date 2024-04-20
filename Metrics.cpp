#include "Metrics.h"

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

void Metrics::read(const int device_type, const u_int64_t num_bytes)
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
    if (instance == nullptr) {
        throw std::runtime_error("Metrics not initialized");
    }

    // choose storage automatically based on available space
    int device_type = Metrics::CURRENT_STORAGE;
    StorageParams & param = instance->params[device_type];
    StorageMetrics & metric = instance->metrics[device_type];
    if (metric.numBytes >= param.capacity) { // Use the next available storage
        if (device_type++ > NUM_STORAGE_TYPES) {
            throw std::runtime_error("No available storage");
        }
        param = instance->params[device_type];
        metric = instance->metrics[device_type];
    }

    // calculate the time spent on data transfer
    metric.dataTransferCost += num_bytes / param.bandwidth;
    metric.numBytes += num_bytes;

    // calculate the fixed latency of the storage system
    metric.accessCost += param.latency;
    metric.numAccesses++;

    return device_type;
}

StorageMetrics Metrics::getMetrics(const int device_type)
{
    if (instance == nullptr) {
        return StorageMetrics();
    }

    if (device_type < 0 || device_type >= NUM_STORAGE_TYPES) {
        return StorageMetrics();
    }

    return instance->metrics[device_type];
}

Metrics::Metrics()
{
    params[STORAGE_SSD] = StorageParams(SSD_LATENCY, SSD_BANDWIDTH);
    params[STORAGE_HDD] = StorageParams(HDD_LATENCY, HDD_BANDWIDTH);

    metrics[STORAGE_SSD] = StorageMetrics();
    metrics[STORAGE_HDD] = StorageMetrics();
}