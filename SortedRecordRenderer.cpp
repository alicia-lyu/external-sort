#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>

SortedRecordRenderer::SortedRecordRenderer (RowSize recordSize, string outputFileName) :
	_recordSize (recordSize), _outputFileName (outputFileName)
{
	TRACE (false);
	_outputBuffer = new Buffer(SSD_PAGE_SIZE, _recordSize);
	_outputFile = ofstream(outputFileName, std::ios::binary);
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
	if (_outputFile.is_open()) {
		_outputFile.write((char *) _outputBuffer->data(), _outputBuffer->sizeFilled());
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
		_addRowToOutputBuffer(row);
    }
	_outputFile.write((char *) _outputBuffer->data(), _outputBuffer->sizeFilled());
	_outputFile.close();
    return _outputFileName;
} // ExternalRenderer::run

byte * SortedRecordRenderer::_addRowToOutputBuffer(byte * row)
{
	byte * output = _outputBuffer->copy(row);
	while (output == nullptr) { // Output buffer is full
		_outputFile.write((char *) _outputBuffer->data(), SSD_PAGE_SIZE);
		output = _outputBuffer->copy(row);
	}
} // SortedRecordRenderer::_addRowToOutputBuffer

NaiveRenderer::NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber) :
	SortedRecordRenderer(recordSize, _getOutputFileName()), _tree(tree), _runNumber (runNumber)
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
	byte * row = _tree->poll();
	_addRowToOutputBuffer(row);
	return row;
} // NaiveRenderer::next

string NaiveRenderer::_getOutputFileName ()
{
	return string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(0) + SEPARATOR + string("run") + std::to_string(_runNumber) + string(".bin");
} // NaiveRenderer::_getOutputFileName

void NaiveRenderer::print ()
{
	_tree->printTree();
} // NaiveRenderer::print

CacheOptimizedRenderer::CacheOptimizedRenderer (RowSize recordSize, vector<TournamentTree *> &cacheTrees, u_int16_t runNumber) : 
	SortedRecordRenderer(recordSize, _getOutputFileName()), _recordSize (recordSize), _cacheTrees (cacheTrees), _runNumber (runNumber)
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

	#if defined(VERBOSEL1) || defined(VERBOSEL2)
	traceprintf ("Formed cache-optimized renderer with %lu cache trees\n", _cacheTrees.size());
	#endif
	#if defined(VERBOSEL2)
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
} // CacheOptimizedRenderer::~CacheOptimizedRenderer

byte * CacheOptimizedRenderer::next ()
{
	u_int16_t bufferNum = _tree->peekTopBuffer();
	TournamentTree * cacheTree = _cacheTrees.at(bufferNum);
	byte * retrieved = cacheTree->poll();
	byte * rendered;
	if (retrieved == nullptr) {
		rendered = _tree->poll();
	} else {
		rendered = _tree->pushAndPoll(retrieved);
	}
	_addRowToOutputBuffer(rendered);
	return rendered;
} // CacheOptimizedRenderer::next

string CacheOptimizedRenderer::_getOutputFileName ()
{
	return string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") + std::to_string(0) + SEPARATOR + string("run") + std::to_string(_runNumber) + string(".bin");
} // CacheOptimizedRenderer::_getOutputFileName

void CacheOptimizedRenderer::print ()
{
    traceprintf ("%zu cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // CacheOptimizedRenderer::print