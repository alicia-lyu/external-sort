#include "Scan.h"
#include "defs.h"

ScanPlan::ScanPlan (RowCount const count, RowSize const size, ofstream_ptr const inFile) : 
	_count (count), _size (size), _inFile (inFile)
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
	return new ScanIterator (this);
} // ScanPlan::init

ScanIterator::ScanIterator (ScanPlan const * const plan) :
	_plan (plan), _count (0)
{
	TRACE (true);
	_row = row(_plan->_size);
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

	std::generate(begin(_row), end(_row), std::ref(_engine));

	++ _count;
	return true;
} // ScanIterator::next

row ScanIterator::getRow ()
{
	TRACE (false);
	unsigned char * rowContent = _row.data();
	traceprintf ("produced %s\n", (unsigned char *) (rowContent));

	ofstream_ptr inFile = _plan->_inFile;
	if (inFile->good()) {
		*inFile << rowContent;
	}

	return _row;
}
