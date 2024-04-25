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
	TRACE (true);
	auto pageSize = Metrics::getParams(_deviceType).pageSize;
	_outputBuffer = new Buffer(pageSize / _recordSize, _recordSize);
	_outputFileName = _getOutputFileName(pass, runNumber);
	_outputFile = ofstream(_outputFileName, std::ios::binary);
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Pass %d run %d: output file %s, device type %d, output page size %d\n", pass, runNumber, _outputFileName.c_str(), _deviceType, pageSize);
	#endif
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
	if (_outputFile.is_open()) {
		bool flush = _flushOutputBuffer(_outputBuffer->sizeFilled());
		if (flush) _outputFile.close();
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
	bool flush = _flushOutputBuffer(_outputBuffer->sizeFilled());
	if (flush) _outputFile.close();
	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Run through renderer with output file %s\n", _outputFileName.c_str());
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
		bool flush = _flushOutputBuffer(_outputBuffer->pageSize);
		if (flush) output = _outputBuffer->copy(row);
	}
	++ _produced;
	return output;
} // SortedRecordRenderer::_addRowToOutputBuffer

string SortedRecordRenderer::_getOutputFileName (u_int8_t pass, u_int16_t runNumber)
{
	string device = string("-device") + std::to_string(_deviceType);
	string dir = string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(pass);
	string filename = string("run") + std::to_string(runNumber) + device;
	return dir + SEPARATOR + filename;
} // SortedRecordRenderer::_getOutputFileName

bool SortedRecordRenderer::_flushOutputBuffer(u_int32_t sizeFilled)
{
	TRACE (false);
	#if defined(VERBOSEL2)
	traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
	#endif
	int deviceType = switchDevice(sizeFilled);
	_outputFile.write((char*) _outputBuffer->data(), sizeFilled);
	Metrics::write(deviceType, sizeFilled);
	return true;
} // SortedRecordRenderer::_flushOutputBuffer

int SortedRecordRenderer::switchDevice (u_int32_t sizeFilled)
{
	if (_deviceType == 0) { // switch device when the current one is SSD and is full
		auto newDeviceType = Metrics::getAvailableStorage(sizeFilled);
		if (newDeviceType != _deviceType) { // switch to a new device
			_deviceType = newDeviceType;
			string newFileName = _outputFileName + string("-") + std::to_string(_produced) + string("-device") + std::to_string(_deviceType);
			std::rename(_outputFileName.c_str(), newFileName.c_str());
			_outputFileName = newFileName;
			delete _outputBuffer;
			_outputBuffer = new Buffer(Metrics::getParams(_deviceType).pageSize / _recordSize, _recordSize);
			_deviceType = newDeviceType;
			#if defined(VERBOSEL1) || defined(VERBOSEL2)
			traceprintf ("Switched to a new device %d at %llu, now output file is %s\n", _deviceType, _produced, _outputFileName.c_str());
			#endif
			return newDeviceType;
		}
	} // Wouldn't switch device when the current one is HDD and SSD becomes available for simplicity (max switch once)
	return _deviceType;
}