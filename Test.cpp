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
	ofstream_ptr inFile = getInFileStream(outputPath);
	u_int32_t maxMemory = 100; // 100MB; 2^32 = 4GB, 10 byte for debugging
	u_int32_t maxRunSize = maxMemory / 2; // 50MB, another 50 MB for output buffer
	u_int32_t recordCountPerRun = maxRunSize / recordSize; // 25 K -- 2.5 M
	u_int16_t runCount = recordCount / recordCountPerRun; // 4000 -- 40
	traceprintf("recordCountPerRun: %u, runCount: %u\n", recordCountPerRun, runCount);

	for (u_int16_t i = 0; i < runCount+1; ++i) {
		u_int32_t recordCountCurrentRun;
		if (i == runCount) {
			recordCountCurrentRun = recordCount - recordCountPerRun * i;
			if (recordCountCurrentRun == 0) break;
		} else {
			recordCountCurrentRun = recordCountPerRun;
		}
		MemoryRun * run = new MemoryRun(recordCountCurrentRun, recordSize);
		Plan * const scanPlan = new ScanPlan (recordCountCurrentRun, recordSize, inFile, run);
		Plan * const witnessPlan = new WitnessPlan (scanPlan, run, recordSize);
		// TODO: Pass run to witnessPlan
		Iterator * const it = witnessPlan->init ();
		it->run ();
		delete it;
		delete scanPlan;
		delete run;
	}

	inFile->close();

	// External sort between memory and SSD


	return 0;
} // main
