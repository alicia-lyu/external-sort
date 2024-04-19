#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"
#include "Verify.h"
#include "Remove.h"

int main (int argc, char * argv [])
{

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes
	string outputPath;
	string inputPath;
	bool removeDuplicate;

	Config config = getArgs(argc, argv);
	recordCount = config.recordCount;
	recordSize = config.recordSize;
	outputPath = config.outputPath;
	inputPath = config.inputPath;
	removeDuplicate = config.removeDuplicates;

	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize);
	Plan * const sortPlan = new SortPlan (witnessPlan, recordSize, recordCount, removeDuplicate);
	Plan * const verifyPlan = new VerifyPlan (sortPlan, recordSize);
	Plan * const witnessPlan2 = new WitnessPlan (verifyPlan, recordSize);

	Iterator * const it = witnessPlan2->init ();
	it->run ();

	// we only delete the last Plan, since it will delete its inner plan
	// in its destructor, and create a chain deletion
	delete it;
	delete witnessPlan2;

	return 0;
} // main
