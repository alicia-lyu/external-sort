#include "SortedRecordRenderer.h"
#include "utils.h"
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <cmath>

SortedRecordRenderer::SortedRecordRenderer ()
{
	TRACE (false);
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (false);
} // SortedRecordRenderer::~SortedRecordRenderer

NaiveRenderer::NaiveRenderer (TournamentTree * tree) :
	_tree (tree)
{
	TRACE (true);
	this->print();
} // NaiveRenderer::NaiveRenderer

NaiveRenderer::~NaiveRenderer ()
{
	TRACE (true);
    delete _tree;
} // NaiveRenderer::~NaiveRenderer

byte * NaiveRenderer::next ()
{
	return _tree->poll();
} // NaiveRenderer::next

void NaiveRenderer::print ()
{
	_tree->printTree();
} // NaiveRenderer::print

CacheOptimizedRenderer::CacheOptimizedRenderer (std::vector<TournamentTree *> cacheTrees, RowSize recordSize) :
	_recordSize (recordSize), _cacheTrees (cacheTrees)
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
	this->print();
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
	u_int8_t bufferNum = _tree->peekTopBuffer();
	TournamentTree * cacheTree = _cacheTrees.at(bufferNum);
	byte * row = cacheTree->poll();
	if (row == nullptr) {
		return _tree->poll();
	} else {
		return _tree->pushAndPoll(row);
	}
} // CacheOptimizedRenderer::next


void CacheOptimizedRenderer::print ()
{
    traceprintf ("%zu cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // CacheOptimizedRenderer::print

ExternalRun::ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize) :
    _runFileName (runFileName), _read (0), _pageSize (pageSize), _recordSize (recordSize), _pageCount (1)
{
    TRACE (true);
    _runFile = std::ifstream(runFileName, std::ios::binary);
    if (!_runFile) {
        throw std::invalid_argument("Run file does not exist");
    }
    inMemoryPage = new Buffer(pageSize / recordSize, recordSize);
    _fillPage();
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (true);
    delete inMemoryPage;
    _runFile.close();
} // ExternalRun::~ExternalRun

byte * ExternalRun::next ()
{
    if (_runFile.eof()) {
        // Reaches end of the run
        traceprintf("End of run file %s\n", _runFileName.c_str());
        return nullptr;
    }
    byte * row = inMemoryPage->next();
    if (row == nullptr) {
        // Reaches end of the page
        _fillPage();
    }
    traceprintf("Read %s from run file %s\n", rowToHexString(row, _recordSize).c_str(), _runFileName.c_str());
    return row;
} // ExternalRun::next

void ExternalRun::_fillPage ()
{
    if (_runFile.eof()) {
        throw std::invalid_argument("Reaches end of the run file unexpectedly.");
    }
    traceprintf("Filling #%d page from run file %s\n", _pageCount, _runFileName.c_str());
    _runFile.read((char *) inMemoryPage->data(), _pageSize);
    _pageCount++;
    inMemoryPage->batchFillByOverwrite(_runFile.gcount());
} // ExternalRun::_fillPage

ExternalRenderer::ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize) :  // 500 KB = 2^19
    _recordSize (recordSize), _pageSize (pageSize)
{
	TRACE (true);
    std::vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, _pageSize, _recordSize);
        _runs.push_back(run);
        formingRows.push_back(run->next());
    }
    _tree = new TournamentTree(formingRows, _recordSize);
    outputBuffer = new Buffer(_pageSize / _recordSize, _recordSize);
	this->print();
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (true);
    delete _tree;
    for (auto run : _runs) {
        delete run;
    }
    delete outputBuffer;
} // ExternalRenderer::~ExternalRenderer

byte * ExternalRenderer::next ()
{
    byte * rendered = _tree->peekRoot();
    // Copy root before calling run.next()
    // For retrieving a new page will overwrite the current page, where root is in
    traceprintf("Output %s\n", rowToHexString(rendered, _recordSize).c_str());
    outputBuffer->copy(rendered);
    // Resume the tournament
	u_int8_t bufferNum = _tree->peekTopBuffer();
    traceprintf("Get the next record from buffer %d\n", bufferNum);
    ExternalRun * run = _runs.at(bufferNum);
    byte * retrieved = run->next();
    if (retrieved == nullptr) {
        _tree->poll();
    } else {
        _tree->pushAndPoll(retrieved);
    }
    return outputBuffer->next();
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%d run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print