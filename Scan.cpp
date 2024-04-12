#include "Scan.h"
#include "utils.h"
#include <memory>

ScanPlan::ScanPlan (RowCount const count, RowSize const size, u_int32_t recordCountPerRun, ofstream_ptr const inFile) : 
	_count (count), _size (size), _inFile (inFile), _countPerRun(recordCountPerRun)
{
	TRACE (true);
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan ()
{
	TRACE (true);
} // ScanPlan::~ScanPlan

Iterator * ScanPlan::init () const
{
	return new ScanIterator (this);
} // ScanPlan::init

ScanIterator::ScanIterator (ScanPlan const * const plan) :
	_plan (plan), _count (0)
{
	TRACE (true);
	_run = new MemoryRun(_plan->_countPerRun, _plan->_size);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator ()
{
	delete _run;
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_count),
			(unsigned long) (_plan->_count));
} // ScanIterator::~ScanIterator

byte * ScanIterator::next ()
{

	if (_count >= _plan->_count)
		return nullptr;

	RowCount runPosition = _count % _plan->_countPerRun;
	byte * row = _run->fillRowRandomly(runPosition);

	string hexString = rowToHexString(row, _plan->_size);
	traceprintf ("produced %s\n", hexString.c_str());
	ofstream_ptr inFile = _plan->_inFile;
	if (inFile->good()) {
		*inFile << hexString << "\n";
	}
	
	++ _count;
	return row;
} // ScanIterator::next