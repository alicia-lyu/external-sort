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

	Plan * const scanPlan = new ScanPlan (7, 20, inFile);
	// Plan * const witnessBefore = new WitnessPlan(scanPlan);
	// TODO: sortPlan
	// Plan * const witnessFinal = new WitnessPlan(scanPlan); // TODO: change to sortPlan

	Iterator * const it = scanPlan->init ();
	it->run ();

	inFile->close();
	
	delete it;
	delete scanPlan;

	return 0;
} // main
