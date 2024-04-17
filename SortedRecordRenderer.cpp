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

// will modify the given cacheTrees
CacheOptimizedRenderer::CacheOptimizedRenderer (vector<TournamentTree *> &cacheTrees, RowSize recordSize) :
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
	u_int16_t bufferNum = _tree->peekTopBuffer();
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