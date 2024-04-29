#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <iostream>
#include <cstdio>

SortedRecordRenderer::SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow, bool materialize) :
	_recordSize (recordSize), _produced (0), _removeDuplicates (removeDuplicates), _lastRow (lastRow)
{
	TRACE (false);
	if (materialize) {
		materializer = new Materializer(pass, runNumber, this);
	} else {
		traceprintf ("Materializer is disabled for pass %d run %d\n", pass, runNumber);
		materializer = nullptr;
	}
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
	if (materializer != nullptr) {
		delete materializer;
	}
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
	traceprintf ("Run through renderer with output file %s\n", _outputFileName.c_str());
	#endif
	if (materializer != nullptr) {
		string outputFileName = materializer->outputFileName;
		delete materializer;
		materializer = nullptr;
		return outputFileName;
	} else {
		throw std::invalid_argument("SortedRecordRenderer::run can only be invoked when Materializer is enabled.");
	}
} // ExternalRenderer::run

byte * SortedRecordRenderer::renderRow(std::function<byte *()> retrieveNext, TournamentTree *& tree, ExternalRun * longRun) 
{
    byte * rendered, * retrieved;
    byte * output = nullptr;

    bool canReturn = false;

	while (true) {
        canReturn = false;

        byte * renderedFromTree = tree->peekRoot();
		byte * renderedFromLongRun = longRun == nullptr ? nullptr : longRun->peek();
		if (renderedFromTree == nullptr && renderedFromLongRun == nullptr) rendered = nullptr;
		else if (renderedFromTree == nullptr) rendered = renderedFromLongRun;
		else if (renderedFromLongRun == nullptr) rendered = renderedFromTree;
		else if (memcmp(renderedFromTree, renderedFromLongRun, _recordSize) <= 0) rendered = renderedFromTree;
		else rendered = renderedFromLongRun;

        // if no more rows, jump out
		if (rendered == nullptr) break;
        
		if (
            !_removeDuplicates ||  // not removing duplicates
            _lastRow == nullptr || // last row is null
            memcmp(_lastRow, rendered, _recordSize) != 0 // last row is different from the current row
        ) {
            if (materializer != nullptr) {
				// copy before retrieving next, as retrieving next could overwrite the current page in ExternalRenderer
				output = materializer->addRowToOutputBuffer(rendered);
				_lastRow = output;
			} else {
				_lastRow = rendered;
				output = rendered;
			}
            canReturn = true;
        } else { // continue to the next row until a distinct one is found
            #if defined(VERBOSEL2)
			traceprintf ("%s removed\n", rowToString(rendered, recordSize).c_str());
			#endif
        }

		if (rendered == renderedFromTree) {
			retrieved = retrieveNext();
			if (retrieved == nullptr) {
				tree->poll();
			} else {
				tree->pushAndPoll(retrieved);
			}
		} else {
			longRun->next();
		}

        if (canReturn) break;
	}

	return output;
}

Materializer::Materializer(u_int8_t pass, u_int16_t runNumber, SortedRecordRenderer * renderer) : 
	 renderer (renderer), deviceType (Metrics::getAvailableStorage())
{
	TRACE (false);
	auto pageSize = Metrics::getParams(deviceType).pageSize;
	outputBuffer = new Buffer(pageSize / renderer->_recordSize, renderer->_recordSize);
	outputFileName = getOutputFileName(pass, runNumber);
	outputFile = ofstream(outputFileName, std::ios::binary);

	#ifdef PRODUCTION
	Trace::PrintTrace(OP_STATE, deviceType == STORAGE_SSD? SPILL_RUNS_SSD : SPILL_RUNS_HDD, 
		string("Spill sorted runs to the ") + getDeviceName(deviceType) + " device");
	#endif

	#if defined(VERBOSEL2)
	traceprintf ("Materializer for run %d with output file %s initialized on device %d\n", runNumber, outputFileName.c_str(), deviceType);
	#endif
} // Materializer::Materializer

Materializer::~Materializer ()
{
	TRACE (false);
	if (outputFile.is_open()) {
		bool flush = flushOutputBuffer(outputBuffer->sizeFilled());
		if (flush) outputFile.close();
	}
	delete outputBuffer;
} // Materializer::~Materializer

string Materializer::getOutputFileName (u_int8_t pass, u_int16_t runNumber)
{
	string device = string("-device") + std::to_string(deviceType);
	string dir = string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(pass);
	string filename = string("run") + std::to_string(runNumber) + device;
	return dir + SEPARATOR + filename;
} // SortedRecordRenderer::_getOutputFileName

byte * Materializer::addRowToOutputBuffer(byte * row)
{
	TRACE (false);
	if (row == nullptr) return nullptr;
	byte * output = outputBuffer->copy(row);
	while (output == nullptr) { // Output buffer is full
		#if defined(VERBOSEL2)
		traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
		#endif
		bool flush = flushOutputBuffer(outputBuffer->pageSize);
		if (flush) output = outputBuffer->copy(row);
	}
	++ renderer->_produced;
	return output;
} // SortedRecordRenderer::_addRowToOutputBuffer

bool Materializer::flushOutputBuffer(u_int32_t sizeFilled)
{
	TRACE (false);
	#if defined(VERBOSEL2)
	traceprintf ("Run %d: output buffer flushed with %llu rows produced\n", _runNumber, _produced);
	#endif

	outputFile.write((char*) outputBuffer->data(), sizeFilled); // Write to file before creating a new buffer
	
	// Metrics: switch device if necessary
	int deviceType = switchDevice(sizeFilled);
	Metrics::write(deviceType, sizeFilled); // However, log the write to the new device
	return true;
} // SortedRecordRenderer::_flushOutputBuffer

int Materializer::switchDevice (u_int32_t sizeFilled)
{
	if (deviceType == STORAGE_SSD) { // switch device when the current one is SSD and is full
		auto newDeviceType = Metrics::getAvailableStorage(sizeFilled);
		if (newDeviceType != deviceType) { // switch to a new device
			deviceType = newDeviceType;
			string newFileName = outputFileName + string("-") + std::to_string(renderer->_produced) + string("-device") + std::to_string(deviceType);
			std::rename(outputFileName.c_str(), newFileName.c_str());
			outputFileName = newFileName;
			delete outputBuffer;
			outputBuffer = new Buffer(Metrics::getParams(deviceType).pageSize / renderer->_recordSize, renderer->_recordSize);
			deviceType = newDeviceType;
			#if defined(VERBOSEL1) || defined(VERBOSEL2)
			traceprintf ("Switched to a new device %d at %llu, now output file is %s\n", deviceType, renderer->_produced, outputFileName.c_str());
			#endif
			return newDeviceType;
		}
	} // Wouldn't switch device when the current one is HDD and SSD becomes available for simplicity (max switch once)
	return deviceType;
}