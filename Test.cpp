#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"
#include "Verify.h"

int main (int argc, char * argv [])
{

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes
	string outputPath;

	std::tie(recordCount, recordSize, outputPath) = getArgs(argc, argv);
	u_int32_t maxMemory = 100000; // Param: 100MB; 2^32 = 4GB, 10 byte for debugging
	u_int32_t maxRunSize = maxMemory / 2; // 50MB, another 50 MB for output buffer
	u_int32_t recordCountPerRun = maxRunSize / recordSize; // 25 K -- 2.5 M
	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, recordCountPerRun);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize);
	Plan * const sortPlan = new SortPlan (witnessPlan, maxRunSize, recordSize, recordCount);
	Plan * const verifyPlan = new VerifyPlan (sortPlan, recordSize);
	Plan * const witnessPlan2 = new WitnessPlan (verifyPlan, recordSize);
	Iterator * const it = witnessPlan2->init ();
	it->run ();

	// we only delete the last Plan, since it will delete its inner plan
	// in its destructor, and so on
	delete it;
	delete witnessPlan2;

	return 0;
} // main
