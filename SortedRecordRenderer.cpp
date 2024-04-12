#include "SortedRecordRenderer.h"
#include <fstream>
#include <algorithm>

SortedRecordRenderer::SortedRecordRenderer ()
{
	TRACE (true);
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (true);
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
	u_int8_t bufferNum = _tree->peek();
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
    _runFileName (runFileName), _read (0), _pageSize (pageSize)
{
    TRACE (true);
    _runFile = std::ifstream(runFileName, std::ios::binary);
    if (!_runFile) {
        throw std::invalid_argument("Run file does not exist");
    }
    inMemoryPage = new Buffer(pageSize / recordSize, recordSize);
    _runFile.read((char *) inMemoryPage->data(), pageSize);
} // ExternalRun::ExternalRun

ExternalRun::~ExternalRun ()
{
    TRACE (true);
    delete inMemoryPage;
    _runFile.close();
} // ExternalRun::~ExternalRun

byte * ExternalRun::next ()
{
    byte * row = inMemoryPage->next();
    if (row == nullptr) { // Reaches end of the run
        traceprintf("End of run file %s\n", _runFileName.c_str());
        return nullptr;
    }
    if (row == inMemoryPage->data()) { // Back to the start of the page---reaches the end of the page
        traceprintf("Getting a new page for run file %s\n", _runFileName.c_str());
        _runFile.read((char *) inMemoryPage->data(), _pageSize);
    }
    return row;
} // ExternalRun::next

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
	this->print();
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (true);
    delete _tree;
    for (auto run : _runs) {
        free(run);
    }
} // ExternalRenderer::~ExternalRenderer

byte * ExternalRenderer::next ()
{
	u_int8_t bufferNum = _tree->peek();
    ExternalRun * run = _runs.at(bufferNum);
    byte * row = run->next();
    if (row == nullptr) {
        return _tree->poll();
    }
    return _tree->pushAndPoll(row);
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%d run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print