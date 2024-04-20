#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <iostream>

SortedRecordRenderer::SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow) :
	_recordSize (recordSize), _produced (0), _runNumber (runNumber), _removeDuplicates (removeDuplicates), _lastRow (lastRow)
{
	TRACE (false);
	_outputBuffer = new Buffer(SSD_PAGE_SIZE / _recordSize, _recordSize);
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
		_outputFile.write((char *) _outputBuffer->data(), _outputBuffer->sizeFilled());
		_outputFile.close();

		// Update metrics
		Metrics::accessStorage(
			Metrics::CURRENT_STORAGE,
			_outputBuffer->sizeFilled()
		);
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
	_outputFile.write(reinterpret_cast<char *>(_outputBuffer->data()), _outputBuffer->sizeFilled());
	_outputFile.close();

	// Update metrics
	Metrics::accessStorage(
		Metrics::CURRENT_STORAGE,
		_outputBuffer->sizeFilled()
	);

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
		output = _outputBuffer->copy(row);

		// Update metrics
		Metrics::accessStorage(
			Metrics::CURRENT_STORAGE,
			_outputBuffer->sizeFilled()
		);
	}
	++ _produced;
	return output;
} // SortedRecordRenderer::_addRowToOutputBuffer

string SortedRecordRenderer::_getOutputFileName (u_int8_t pass, u_int16_t runNumber)
{
	return string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(pass) + SEPARATOR + string("run") + std::to_string(runNumber) + string(".bin");
} // SortedRecordRenderer::_getOutputFileName

NaiveRenderer::NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber, bool removeDuplicates) :
	SortedRecordRenderer(recordSize, 0, runNumber, removeDuplicates), _tree(tree)
{
	TRACE (true);
} // NaiveRenderer::NaiveRenderer

NaiveRenderer::~NaiveRenderer ()
{
	TRACE (true);
    delete _tree;
} // NaiveRenderer::~NaiveRenderer

byte * NaiveRenderer::next ()
{
	byte * rendered = renderRow(
		[this] () -> byte * {
			return nullptr;
		},
		[this] (byte * rendered) -> byte * {
			return _addRowToOutputBuffer(rendered);
		},
		_tree,
		_lastRow,
		_removeDuplicates,
		_recordSize
	);
	_lastRow = rendered;
	return rendered;
} // NaiveRenderer::next

void NaiveRenderer::print ()
{
	_tree->printTree();
} // NaiveRenderer::print

CacheOptimizedRenderer::CacheOptimizedRenderer (RowSize recordSize, 
	vector<TournamentTree *> &cacheTrees, u_int16_t runNumber, bool removeDuplicates) : 
	SortedRecordRenderer(recordSize, 0, runNumber, removeDuplicates), _cacheTrees (cacheTrees)
{
	TRACE (true);
    std::vector<byte *> formingRows;
    for (auto cacheTree : _cacheTrees) {
        byte * row = cacheTree->poll();
        if (row == nullptr) {
            throw std::invalid_argument("Cache tree is empty");
        }
        formingRows.push_back(row);
    }
    _tree = new TournamentTree(formingRows, _recordSize);

	lastRow = nullptr;
	_removed = 0;

	#if VERBOSEL1 || VERBOSEL2
	traceprintf ("Formed cache-optimized renderer with %lu cache trees\n", _cacheTrees.size());
	#endif
	#if VERBOSEL2
	this->print();
	#endif
} // CacheOptimizedRenderer::CacheOptimizedRenderer

CacheOptimizedRenderer::~CacheOptimizedRenderer ()
{
	TRACE (true);
    delete _tree;
    for (auto cacheTree : _cacheTrees) {
        delete cacheTree;
    }

	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf("Produced %llu rows\n", _produced);
	if (_removeDuplicates) {
		traceprintf ("Removed %llu rows\n", _removed);
	}
	#endif
} // CacheOptimizedRenderer::~CacheOptimizedRenderer

byte * CacheOptimizedRenderer::next ()
{
	byte * rendered = renderRow(
		[this] () -> byte * {
			auto bufferNum = _tree->peekTopBuffer();
			auto cacheTree = _cacheTrees.at(bufferNum);
			return cacheTree->poll();
		},
		[this] (byte * rendered) -> byte * {
			return _addRowToOutputBuffer(rendered);
		},
		_tree,
		_lastRow,
		_removeDuplicates,
		_recordSize
	);
	_lastRow = rendered;
	return rendered;
} // CacheOptimizedRenderer::next

void CacheOptimizedRenderer::print ()
{
    traceprintf ("%zu cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // CacheOptimizedRenderer::print