#include "Iterator.h"
#include "Scan.h"
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

	Config config = getArgs(argc, argv);
	recordCount = config.recordCount;
	recordSize = config.recordSize;

	bool removeDuplicate = config.removeDuplicates;
	string & removalMethod = config.removalMethod;
	bool useInsort = removeDuplicate && removalMethod == "insort";
	bool useInstream = removeDuplicate && removalMethod == "instream";

	// trace log file
	string & tracePath = config.tracePath;
	Trace::SetOutputFd(STDOUT_FILENO);
	if (!tracePath.empty())
		freopen(tracePath.c_str(), "w+", stdout);

	// input file
	string & inputPath = config.inputPath;

	// u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	// traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);
	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, inputPath);
	Plan * const witnessPlan = new WitnessPlan (scanPlan, recordSize, false);
	Plan * const sortPlan = new SortPlan (witnessPlan, recordSize, recordCount, useInsort);
	Plan * const removePlan = new InStreamRemovePlan (sortPlan, recordSize, useInstream);
	Plan * const verifyPlan = new VerifyPlan (removePlan, recordSize);
	Plan * const witnessPlan2 = new WitnessPlan (verifyPlan, recordSize, true);

	Iterator * const it = witnessPlan2->init ();
	it->run ();

	// we only delete the last Plan, since it will delete its inner plan
	// in its destructor, and create a chain deletion
	delete it;
	delete witnessPlan2;

	renameOutputFile(config.outputPath);

	// print the metrics
	auto ssdMetrics = Metrics::getMetrics(STORAGE_SSD);
	auto ssdParams = Metrics::getParams(STORAGE_SSD);
	auto hddMetrics = Metrics::getMetrics(STORAGE_HDD);
	auto hddParams = Metrics::getParams(STORAGE_HDD);

	Trace::PrintTrace(OP_RESULT, METRICS_RESULT, 
		"SSD: dataTransferCost: " + to_string(ssdMetrics.dataTransferCost) +
		", expected data transfer cost: " + to_string((ssdMetrics.numBytesRead + ssdMetrics.numBytesWritten) / (double) ssdParams.bandwidth) +
		"\naccessCost: " + to_string(ssdMetrics.accessCost) +
		", expected access cost: " + to_string((ssdMetrics.numAccessesRead + ssdMetrics.numAccessesWritten) * ssdParams.latency));

	Trace::PrintTrace(OP_RESULT, METRICS_RESULT,
		"HDD: dataTransferCost: " + to_string(hddMetrics.dataTransferCost) +
		", expected data transfer cost: " + to_string((hddMetrics.numBytesRead + hddMetrics.numBytesWritten) / (double) hddParams.bandwidth) +
		"\naccessCost: " + to_string(hddMetrics.accessCost) +
		", expected access cost: " + to_string((hddMetrics.numAccessesRead + hddMetrics.numAccessesWritten) * hddParams.latency));
	
	// restore stdout
	fflush(stdout);
	Trace::ResumeStdout();

	return 0;
} // main
