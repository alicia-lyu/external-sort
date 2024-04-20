#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <iostream>

SortedRecordRenderer::SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow) :
	_recordSize (recordSize), _runNumber (runNumber), _produced (0), _removeDuplicates (removeDuplicates), _lastRow (lastRow)
{
	TRACE (false);
	auto device_type = Metrics::getAvailableStorage();
	auto page_size = Metrics::getParams(device_type).pageSize;
	_outputBuffer = new Buffer(page_size / _recordSize, _recordSize);
	_outputFileName = _getOutputFileName(pass, runNumber);
	_outputFile = ofstream(_outputFileName, std::ios::binary);
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Run %d: output file %s\n", runNumber, _outputFileName.c_str());
	#endif
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
	if (_outputFile.is_open()) {
		_write(_outputBuffer->sizeFilled());
		_outputFile.close();
	}
	delete _outputBuffer;
} // SortedRecordRenderer::~SortedRecordRenderer

string SortedRecordRenderer::run ()
{
    TRACE (false);
    byte * row = next();
    while (row != nullptr) {
        row = next();
    }
	#if defined(VERBOSEL2)
	traceprintf ("%s: produced %llu rows\n", _outputFileName.c_str(), _produced);
	#endif
	_write(SSD_PAGE_SIZE);
	_outputFile.close();
    return _outputFileName;
} // ExternalRenderer::run

byte * SortedRecordRenderer::_addRowToOutputBuffer(byte * row)
{
	if (row == nullptr) return nullptr;
	byte * output = _outputBuffer->copy(row);
	while (output == nullptr) { // Output buffer is full
		_outputFile.write(reinterpret_cast<char *>(_outputBuffer->data()), SSD_PAGE_SIZE);
		#if defined(VERBOSEL2)
		traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
		#endif
		_write(SSD_PAGE_SIZE);
	}
	++ _produced;
	return output;
} // SortedRecordRenderer::_addRowToOutputBuffer

string SortedRecordRenderer::_getOutputFileName (u_int8_t pass, u_int16_t runNumber)
{
	return string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(pass) + SEPARATOR + string("run") + std::to_string(runNumber) + string(".bin");
} // SortedRecordRenderer::_getOutputFileName

void SortedRecordRenderer::_write(u_int16_t sizeFilled)
{
	_outputFile.write((char*) _outputBuffer->data(), sizeFilled);
	#if defined(VERBOSEL2)
	traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
	#endif
	Metrics::write(sizeFilled);
} // SortedRecordRenderer::_write