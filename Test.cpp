#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"
#include "Witness.h"

int main (int argc, char * argv [])
{
	TRACE (true);

	RowCount recordCount;
	RowSize recordSize; // 20-2000 bytes
	string outputPath;

	std::tie(recordCount, recordSize, outputPath) = getArgs(argc, argv);
	ofstream_ptr inFile = getInFileStream(outputPath);

	MemoryRun * run = new MemoryRun(recordCount, recordSize);
	// TODO: separate count in memory run and in total (later)

	Plan * const scanPlan = new ScanPlan (recordCount, recordSize, inFile, run);
	// Plan * const witnessBefore = new WitnessPlan(scanPlan);
	// TODO: sortPlan
	// Plan * const witnessFinal = new WitnessPlan(witnessBefore); // TODO: change to sortPlan

	Iterator * const it = scanPlan->init ();
	it->run ();

	inFile->close();
	
	delete it;
	delete scanPlan;
	delete run;

	return 0;
} // main
