#define CACHE_SIZE 1000000 // Change to 1 MB
#define MEMORY_SIZE 100000000 // Change to 100 MB
#define SSD_SIZE 10000000000 // Change to 10 GB
// HDD size: unlimited

#define SSD_PAGE_SIZE 20000 // Change to 20 KB
#define HDD_PAGE_SIZE 500000 // Change to 500 KB

// ============ Metrics ============
#define SSD_BANDWIDTH 200000000 // Change to 200 MB/s
#define HDD_BANDWIDTH 100000000 // Change to 100 MB/s

#define SSD_LATENCY 0.0001 // Change to 0.1 ms
#define HDD_LATENCY 0.005 // Change to 5 ms

#define STORAGE_SSD 0
#define STORAGE_HDD 1

#define NUM_STORAGE_TYPES 2

// device names
#define DEVICE_SSD "SSD"
#define DEVICE_HDD "HDD"
#define getDeviceName(device_type) (device_type == STORAGE_SSD ? DEVICE_SSD : DEVICE_HDD)

// ==========================

#define READ_AHEAD_BUFFERS_MIN 2
#define GRACEFUL_DEGRADATION_THRESHOLD 1.5
#define LONG_RUN_THRESHOLD 2

#define MAX_INPUT_FILE_SIZE 120000000000 // Not curtail input file size