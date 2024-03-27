#include "Iterator.h"
#include "Scan.h"
#include "Filter.h"
#include "Sort.h"
#include "utils.h"

int main (int argc, char * argv [])
{
	TRACE (true);

		RowCount recordCount;
	    RowSize recordSize; // 20-2000 bytes
	    string outputPath;

		std::tie(recordCount, recordSize, outputPath) = getArgs(argc, argv);
		ofstream_ptr inFile = getInFileStream(outputPath);

		Plan * const plan = new ScanPlan (7, 20, inFile);
		// new SortPlan ( new FilterPlan ( new ScanPlan (7) ) );

		Iterator * const it = plan->init ();
		it->run ();

		inFile->close();
		
		delete it;

		delete plan;

	return 0;
} // main
