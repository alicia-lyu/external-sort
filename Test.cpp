#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"
#include "Verify.h"
#include "Remove.h"
#include "Metrics.h"

int main (int argc, char * argv [])
{
	Metrics::Init();

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

	// print the metrics
	auto ssdMetrics = Metrics::getMetrics(STORAGE_SSD);
	auto hddMetrics = Metrics::getMetrics(STORAGE_HDD);
	traceprintf("SSD accessed %llu times and transferred %llu bytes\n",
		ssdMetrics.numAccesses, ssdMetrics.numBytes);
	traceprintf("SSD total latency: %.2f (ms), total IO latency: %.2f (ms)\n",
		ssdMetrics.accessCost * 1000, ssdMetrics.dataTransferCost * 1000);
	traceprintf("HDD accessed %llu times and transferred %llu bytes\n",
		hddMetrics.numAccesses, hddMetrics.numBytes);
	traceprintf("HDD total latency: %.2f (ms), total IO latency: %.2f (ms)\n",
		hddMetrics.accessCost * 1000, hddMetrics.dataTransferCost * 1000);

	return 0;
} // main
