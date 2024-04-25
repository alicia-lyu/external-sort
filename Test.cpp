#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"
#include "Verify.h"
#include "Remove.h"
#include "Metrics.h"
#include <unistd.h>

int main (int argc, char * argv [])
{
	Metrics::Init();

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes
	bool removeDuplicate;

	Config config = getArgs(argc, argv);
	recordCount = config.recordCount;
	recordSize = config.recordSize;
	removeDuplicate = config.removeDuplicates;

	// output log file
	string & outputPath = config.outputPath;
	int stdout_copy = dup(STDOUT_FILENO);
	if (!outputPath.empty()) {
		freopen(outputPath.c_str(), "w+", stdout);
	}

	// input file
	string & inputPath = config.inputPath;

	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, inputPath);
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
	auto ssdParams = Metrics::getParams(STORAGE_SSD);
	auto hddMetrics = Metrics::getMetrics(STORAGE_HDD);
	auto hddParams = Metrics::getParams(STORAGE_HDD);

	traceprintf("SSD: dataTransferCost: %f, expected data transfer cost: %f\naccessCost: %f, expected access cost: %f\n",
		ssdMetrics.dataTransferCost,
		(ssdMetrics.numBytesRead + ssdMetrics.numBytesWritten) / (double) ssdParams.bandwidth,
		ssdMetrics.accessCost,
		(ssdMetrics.numAccessesRead + ssdMetrics.numAccessesWritten) * ssdParams.latency
	);
	traceprintf("HDD: dataTransferCost: %f, expected data transfer cost: %f\naccessCost: %f, expected access cost: %f\n",
		hddMetrics.dataTransferCost,
		(hddMetrics.numBytesRead + hddMetrics.numBytesWritten) / (double) hddParams.bandwidth,
		hddMetrics.accessCost,
		(hddMetrics.numAccessesRead + hddMetrics.numAccessesWritten) * hddParams.latency
	);
	
	// restore stdout
	if (!outputPath.empty()) {
		fflush(stdout);
		dup2(stdout_copy, STDOUT_FILENO);
		close(stdout_copy);
	}

	return 0;
} // main
