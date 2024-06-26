#include "Scan.h"
#include "utils.h"
#include <memory>

ScanPlan::ScanPlan (RowCount const count, RowSize const size, const string &inputPath) : 
	_count (count), _size (size), _inputPath(inputPath)
{
	TRACE (false);
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
	countPerRun = getRecordCountPerRun(plan->_size);
	if (_plan->_inputPath.empty()) {
		_run = new RandomBuffer(countPerRun, _plan->_size);
	} else {
		_run = new InFileBuffer(countPerRun, _plan->_size, _plan->_inputPath);
	}
	_inputFile = std::ofstream(_getInputFileName(), std::ios::binary);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator ()
{
	TRACE (false);
	delete _run;
	_inputFile.close();
	#if defined(VERBOSEL2) || defined(VERBOSEL1)
	traceprintf ("produced %lu of %lu rows\n",
			(unsigned long) (_count),
			(unsigned long) (_plan->_count));
	#endif
} // ScanIterator::~ScanIterator

byte * ScanIterator::next ()
{

	if (_count >= _plan->_count)
		return nullptr;

	byte * row = _run->next();
	if (row == nullptr) {
		// The current buffer is exhausted
		auto countPerRunNew = getRecordCountPerRun(_plan->_size);
		if (countPerRunNew != countPerRun) {
			delete _run;
			if (_plan->_inputPath.empty()) {
				_run = new RandomBuffer(countPerRunNew, _plan->_size);
			} else {
				throw std::runtime_error("Input file option is only implemented for in-memory sort. Please let the program generate random data.");
			}
			countPerRun = countPerRunNew;
		}
		row = _run->next();
	}

	RowCount rowCountInCurrentScan = _count - _scanCount * _countPerScan;
	if (rowCountInCurrentScan >= _countPerScan) {
		_inputFile.close();
		_inputFile = std::ofstream(_getInputFileName(), std::ios::binary);
		_scanCount++;
	}
	if (_inputFile.good()) {
		_inputFile.write((char *) row, _plan->_size);
		_inputFile.write("\n", 1);
	} else {
		throw std::runtime_error("Error writing to input file scan" + std::to_string(_scanCount));
	}
	#ifdef VERBOSEL1
	if (_count % 10000 == 0) traceprintf ("# %llu produced %s\n", _count, rowToString(row, _plan->_size).c_str());
	#endif
	++ _count;
	return row;
} // ScanIterator::next

string ScanIterator::_getInputFileName()
{
	return std::string(".") + SEPARATOR + std::string("inputs") + SEPARATOR + std::string("input") + std::to_string(_scanCount) + std::string(".txt");
} // ScanIterator::_getInputFileName

bool ScanIterator::forceFlushBuffer()
{
	Trace::PrintStdout("Buffer forced to flush %d / %d.\n", _run->sizeFilled(), _run->pageSize);
	delete _run;
	auto countPerRunNew = getRecordCountPerRun(_plan->_size);
	if (_plan->_inputPath.empty()) {
		_run = new RandomBuffer(countPerRunNew, _plan->_size);
	} else {
		throw std::runtime_error("Input file option is only implemented for in-memory sort. Please let the program generate random data.");
	}
	countPerRun = countPerRunNew;
	return true;
} // ScanIterator::forceFlushBuffer