#include "SortedRecordRenderer.h"
#include <fstream>


SortedRecordRenderer::SortedRecordRenderer (
	TournamentTree * tree, 
	std::vector<TournamentTree *> cacheTrees, 
	std::vector<string> runFileNames) :
	_tree (tree), _cacheTrees (cacheTrees), _runFileNames (runFileNames)
{
	TRACE (true);
	if (_cacheTrees.size() != 0 && _runFileNames.size() != 0) {
		throw std::invalid_argument("Only either cache trees or in-file runs can be provided");
	}
	this->print();
} // SortedRecordRenderer::SortedRecordRenderer

SortedRecordRenderer::~SortedRecordRenderer ()
{
	TRACE (true);
	delete _tree;
	for (auto cacheTree : _cacheTrees) {
		delete cacheTree;
	}
} // SortedRecordRenderer::~SortedRecordRenderer

byte * SortedRecordRenderer::next ()
{
	if (_cacheTrees.size() == 0 && _runFileNames.size() == 0) {
		// No cache trees, we only have one huge tree for all records in memory
		return _tree->poll();
	} else if (_runFileNames.size() == 0) {
		// We have cache trees, we have to merge them along the way of polling the huge tree
		u_int8_t bufferNum = _tree->peek();
		TournamentTree * cacheTree = _cacheTrees.at(bufferNum);
		byte * row = cacheTree->poll();
		if (row == nullptr) {
			return _tree->poll();
		} else {
			return _tree->pushAndPoll(row);
		}
	} else {
		// TODO: We have in-file runs, we have to merge them along the way of polling the huge tree
		return nullptr;
	}
} // SortedRecordRenderer::next

void SortedRecordRenderer::print ()
{
	traceprintf ("%d cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // SortedRecordRenderer::print