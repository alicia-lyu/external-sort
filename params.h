#define CACHE_SIZE 1000 // Change to 1 MB
#define MEMORY_SIZE 100000 // Change to 100 MB
#define SSD_SIZE 10000000 // Change to 10 GB
// HDD size: unlimited

#define SSD_PAGE_SIZE 2000 // Change to 20 KB
#define HDD_PAGE_SIZE 20000 // Change to 500 KB

// ============ Metrics ============
#define SSD_BANDWIDTH 200000 // Change to 200 MB/s
#define HDD_BANDWIDTH 100000 // Change to 100 MB/s

#define SSD_LATENCY 0.0001 // Change to 0.1 ms
#define HDD_LATENCY 0.005 // Change to 5 ms

#define STORAGE_SSD 0
#define STORAGE_HDD 1

#define NUM_STORAGE_TYPES 2
// ==========================

#define READ_AHEAD_BUFFERS_MIN 2