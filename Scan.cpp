#include "Scan.h"
#include "utils.h"
#include <memory>

ScanPlan::ScanPlan (RowCount const count, RowSize const size, const string &inputPath) : 
	_count (count), _size (size), _countPerRun (getRecordCountPerRun(size, true)), _inputPath(inputPath)
{
	TRACE (false);
	// CHECK WITH TA: Do we need to evaluate the metrics of data read & final write? 
	// Since they are the same across different implementation, may as well only evaluate the spills
	// This way, we also don't need to modify Scan when introducing HDD
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan ()
{
	TRACE (false);
} // ScanPlan::~ScanPlan

Iterator * ScanPlan::init () const
{
	return new ScanIterator (this);
} // ScanPlan::init

ScanIterator::ScanIterator (ScanPlan const * const plan) :
	_plan (plan), _scanCount(0), _countPerScan(MAX_INPUT_FILE_SIZE / plan->_size), _count (0)
{
	TRACE (false);
	// if input path is not empty, use InFileBuffer
	// else, use RandomBuffer
	if (_plan->_inputPath.empty()) {
		_run = new RandomBuffer(_plan->_countPerRun, _plan->_size);
	} else {
		_run = new InFileBuffer(_plan->_countPerRun, _plan->_size, _plan->_inputPath);
	}
	_inputFile = std::ofstream(_getInputFileName(), std::ios::binary);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator ()
{
	TRACE (false);
	delete _run;
	_inputFile.close();
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_count),
			(unsigned long) (_plan->_count));
} // ScanIterator::~ScanIterator

byte * ScanIterator::next ()
{

	if (_count >= _plan->_count)
		return nullptr;

	byte * row;
	do {
		row = _run->next();
	} while (row == nullptr);

	#ifdef VERBOSEL2
	traceprintf ("%d produced %s\n", _count, rowToString(row, _plan->_size).c_str());
	#endif

	RowCount rowCountInCurrentScan = _count - _scanCount * _countPerScan;
	if (rowCountInCurrentScan >= _countPerScan) {
		_inputFile.close();
		_inputFile = std::ofstream(_getInputFileName(), std::ios::binary);
		_scanCount++;
	}
	if (_inputFile.good()) {
		_inputFile.write(reinterpret_cast<char *>(row), _plan->_size);
	} else {
		throw std::runtime_error("Error writing to input file scan" + std::to_string(_scanCount));
	}
	++ _count;
	return row;
} // ScanIterator::next

string ScanIterator::_getInputFileName()
{
	return std::string(".") + SEPARATOR + std::string("inputs") + SEPARATOR + std::string("scan") + std::to_string(_scanCount) + std::string(".bin");
} // ScanIterator::_getInputFileName