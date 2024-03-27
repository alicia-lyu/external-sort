#include "Scan.h"
#include "utils.h"
#include <memory>

ScanPlan::ScanPlan (RowCount const count, RowSize const size, ofstream_ptr const inFile, MemoryRun * run) : 
	_count (count), _size (size), _inFile (inFile), _run (run)
{
	TRACE (true);
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan ()
{
	TRACE (true);
} // ScanPlan::~ScanPlan

Iterator * ScanPlan::init () const
{
	TRACE (true);
	return new ScanIterator (this, _run);
} // ScanPlan::init

ScanIterator::ScanIterator (ScanPlan const * const plan, MemoryRun * run) :
	_plan (plan), _count (0), _run (run)
{
	TRACE (true);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator ()
{
	TRACE (true);
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_count),
			(unsigned long) (_plan->_count));
} // ScanIterator::~ScanIterator

bool ScanIterator::next ()
{
	TRACE (false);

	if (_count >= _plan->_count)
		return false;

	byte * row = _run->fillRowRandomly(_count);
	// TODO: separate count in plan and in memory run, when memory spills
	// TODO: use promise to ensure row is ready before next() returns

	string hexString = rowToHexString(row, _plan->_size);
	traceprintf ("produced %s\n", hexString.c_str());
	ofstream_ptr inFile = _plan->_inFile;
	if (inFile->good()) {
		*inFile << hexString << "\n";
	}
	
	++ _count;
	return true;
} // ScanIterator::next