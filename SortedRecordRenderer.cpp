#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <cstdio>

SortedRecordRenderer::SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow) :
	_recordSize (recordSize), _runNumber (runNumber), _produced (0), _removeDuplicates (removeDuplicates), _lastRow (lastRow), _deviceType (Metrics::getAvailableStorage())
{
	TRACE (false);
	auto pageSize = Metrics::getParams(_deviceType).pageSize;
	_outputBuffer = new Buffer(pageSize / _recordSize, _recordSize);
	_outputFileName = _getOutputFileName(pass, runNumber);
	_outputFile = ofstream(_outputFileName, std::ios::binary);
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Run %d: output file %s, device type %d, page size %d\n", runNumber, _outputFileName.c_str(), device_type, page_size);
	#endif
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
	if (_outputFile.is_open()) {
		_flushOutputBuffer(_outputBuffer->sizeFilled());
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
    return _outputFileName;
} // ExternalRenderer::run

byte * SortedRecordRenderer::_addRowToOutputBuffer(byte * row)
{
	TRACE (false);
	if (row == nullptr) return nullptr;
	byte * output = _outputBuffer->copy(row);
	while (output == nullptr) { // Output buffer is full
		#if defined(VERBOSEL2)
		traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
		#endif
		_flushOutputBuffer(_outputBuffer->pageSize);
		output = _outputBuffer->copy(row);
	}
	++ _produced;
	return output;
} // SortedRecordRenderer::_addRowToOutputBuffer

string SortedRecordRenderer::_getOutputFileName (u_int8_t pass, u_int16_t runNumber)
{
	string device = string("-device") + std::to_string(Metrics::CURRENT_STORAGE);
	string dir = string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(pass);
	string filename = string("run") + std::to_string(runNumber) + device;
	return dir + SEPARATOR + filename;
} // SortedRecordRenderer::_getOutputFileName

void SortedRecordRenderer::_flushOutputBuffer(u_int16_t sizeFilled)
{
	TRACE (false);
	_outputFile.write((char*) _outputBuffer->data(), sizeFilled);
	auto newDeviceType = Metrics::getAvailableStorage();
	if (newDeviceType != _deviceType) { // switch to a new device
		_deviceType = newDeviceType;
		string newFileName = _outputFileName + std::to_string(_produced) + string("-device") + std::to_string(_deviceType);
		std::rename(_outputFileName.c_str(), newFileName.c_str());
		_outputFileName = newFileName;
		delete _outputBuffer;
		_outputBuffer = new Buffer(Metrics::getParams(_deviceType).pageSize / _recordSize, _recordSize);
		#if defined(VERBOSEL1) || defined(VERBOSEL2)
		traceprintf ("Switched to a new device %d at %llu, now output file is %s\n", _deviceType, _produced, _outputFileName.c_str());
		#endif
	}
	#if defined(VERBOSEL2)
	traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
	#endif
	Metrics::write(sizeFilled);
} // SortedRecordRenderer::_flushOutputBuffer