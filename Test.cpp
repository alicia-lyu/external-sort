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

	std::tie(recordCount, recordSize, outputPath) = getArgs(argc, argv);
	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize);
	Plan * const sortPlan = new SortPlan (witnessPlan, recordSize, recordCount);
	Plan * const witnessPlan2 = new WitnessPlan (sortPlan, recordSize);
	Iterator * const it = witnessPlan2->init ();
	it->run ();
	delete it;
	delete witnessPlan2;

	return 0;
} // main
