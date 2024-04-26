#include "Iterator.h"
#include "Scan.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"
#include "Verify.h"
#include "Remove.h"
#include "Metrics.h"
#include "Output.h"
#include <unistd.h>

int main (int argc, char * argv [])
{
	Metrics::Init();

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes

	Config config = getArgs(argc, argv);
	recordCount = config.recordCount;
	recordSize = config.recordSize;

	bool removeDuplicate = config.removeDuplicates;
	string & removalMethod = config.removalMethod;
	bool useInsort = removeDuplicate && removalMethod == "insort";
	bool useInstream = removeDuplicate && removalMethod == "instream";

	// trace log file
	string & tracePath = config.tracePath;
	int stdout_copy = dup(STDOUT_FILENO);
	if (!tracePath.empty())
		freopen(tracePath.c_str(), "w+", stdout);

	// input file
	string & inputPath = config.inputPath;

	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, inputPath);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize);
	Plan * const sortPlan = new SortPlan (witnessPlan, recordSize, recordCount, useInsort);
	Plan * const removePlan = new InStreamRemovePlan (sortPlan, recordSize, useInstream);
	Plan * const verifyPlan = new VerifyPlan (removePlan, recordSize);
	Plan * const witnessPlan2 = new WitnessPlan (verifyPlan, recordSize);
	Plan * const outputPlan = new OutputPlan (witnessPlan2, recordSize, config.outputPath);

	Iterator * const it = outputPlan->init ();
	it->run ();

	// we only delete the last Plan, since it will delete its inner plan
	// in its destructor, and create a chain deletion
	delete it;
	delete outputPlan;

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
	fflush(stdout);
	dup2(stdout_copy, STDOUT_FILENO);
	close(stdout_copy);

	return 0;
} // main
