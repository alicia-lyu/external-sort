#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"

int main (int argc, char * argv [])
{

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes
	string outputPath;

	traceprintf("Generating data and creating memory runs\n");
	std::tie(recordCount, recordSize, outputPath) = getArgs(argc, argv);
	u_int32_t maxMemory = 1000000; // 100MB; 2^32 = 4GB, 10 byte for debugging
	u_int32_t maxRunSize = maxMemory / 2; // 50MB, another 50 MB for output buffer
	u_int32_t recordCountPerRun = maxRunSize / recordSize; // 25 K -- 2.5 M
	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, recordCountPerRun);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize);
	Plan * const sortPlan = new SortPlan (witnessPlan, recordCountPerRun, recordSize, recordCount);
	Plan * const witnessPlan2 = new WitnessPlan (sortPlan, recordSize);
	Iterator * const it = witnessPlan2->init ();
	it->run ();
	delete it;
	delete witnessPlan2;

	return 0;
} // main
