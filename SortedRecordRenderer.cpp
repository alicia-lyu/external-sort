#include "SortedRecordRenderer.h"
#include <fstream>

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

ExternalRenderer::ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize) :  // 500 KB = 2^19
	_runFileNames (runFileNames), _recordSize (recordSize), _pageSize (pageSize),
    _runCount (runFileNames.size()), _currentPages (std::vector<int>(_runCount, 1)), _currentRecords (std::vector<int>(_runCount, 1))
{
	TRACE (true);
    std::vector<byte *> formingRows;
    for (auto runFileName : _runFileNames) {
        std::ifstream runFile(runFileName, std::ios::binary);
        if (!runFile) {
            throw std::invalid_argument("Run file does not exist");
        }
        byte * page = (byte *) malloc(pageSize * recordSize * sizeof(byte));
        formingRows.push_back(page);
        runFile.read((char *) page, pageSize * recordSize);
        _pages.push_back(page);
        runFile.close();
    }
    _tree = new TournamentTree(formingRows, _recordSize);
	this->print();
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (true);
    delete _tree;
    for (auto page : _pages) {
        free(page);
    }
} // ExternalRenderer::~ExternalRenderer

byte * ExternalRenderer::next ()
{
	u_int8_t bufferNum = _tree->peek();
    byte * page = _pages.at(bufferNum);
    int currentRecord = _currentRecords.at(bufferNum);
    if (currentRecord * _recordSize >= _pageSize) {
        int currentPage = _currentPages.at(bufferNum);
        if (currentPage * _pageSize >= std::filesystem::file_size(_runFileNames.at(bufferNum))) { // no new pages
            return _tree->poll();
        }
        std::ifstream runFile(_runFileNames.at(bufferNum), std::ios::binary);
        runFile.seekg(currentPage * _pageSize * _recordSize).read((char *) page, _pageSize * _recordSize);
        runFile.close();
        _currentPages.at(bufferNum)++;
    }
    byte * row = page + currentRecord * _recordSize;
    _currentRecords.at(bufferNum)++;
    return _tree->pushAndPoll(row);
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%d run files\n", _runCount);
	_tree->printTree();
} // ExternalRenderer::print