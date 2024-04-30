#include "InMemoryRenderer.h"
#include "utils.h"

NaiveRenderer::NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber, bool removeDuplicates, bool materialize) :
	SortedRecordRenderer(recordSize, 0, runNumber, removeDuplicates, nullptr, materialize), _tree(tree)
{
	TRACE (false);
} // NaiveRenderer::NaiveRenderer

NaiveRenderer::~NaiveRenderer ()
{
	TRACE (false);
    delete _tree;
} // NaiveRenderer::~NaiveRenderer

byte * NaiveRenderer::next ()
{
	return SortedRecordRenderer::renderRow(
		[] () -> byte * {
			return nullptr;
		},
		_tree
	);
} // NaiveRenderer::next

void NaiveRenderer::print ()
{
	_tree->printTree();
} // NaiveRenderer::print

CacheOptimizedRenderer::CacheOptimizedRenderer (RowSize recordSize, 
	vector<TournamentTree *> &cacheTrees, u_int16_t runNumber, 
	bool removeDuplicates, bool materialize) 
: 
	SortedRecordRenderer(recordSize, 0, runNumber, removeDuplicates, nullptr, materialize), _cacheTrees (cacheTrees)
{
	TRACE (false);
    std::vector<byte *> formingRows;
    for (auto cacheTree : _cacheTrees) {
        byte * row = cacheTree->poll();
        if (row == nullptr) {
            throw std::invalid_argument("Cache tree is empty");
        }
        formingRows.push_back(row);
    }
    _tree = new TournamentTree(formingRows, _recordSize);

	#ifdef PRODUCTION
	Trace::PrintTrace(OP_STATE, SORT_MINI_RUNS, "Sort cache-size mini runs");
	#endif

	#if VERBOSEL1
	this->print();
	#endif
} // CacheOptimizedRenderer::CacheOptimizedRenderer

CacheOptimizedRenderer::~CacheOptimizedRenderer ()
{
	TRACE (false);
    delete _tree;
    for (auto cacheTree : _cacheTrees) {
        delete cacheTree;
    }

	#if defined(VERBOSEL2)
	traceprintf("Produced %llu rows\n", _produced);
	#endif
} // CacheOptimizedRenderer::~CacheOptimizedRenderer

byte * CacheOptimizedRenderer::next ()
{
	return SortedRecordRenderer::renderRow(
		[this] () -> byte * {
			auto bufferNum = _tree->peekTopBuffer();
			auto cacheTree = _cacheTrees.at(bufferNum);
			return cacheTree->poll();
		},
		_tree
	);
} // CacheOptimizedRenderer::next

void CacheOptimizedRenderer::print ()
{
    traceprintf ("%zu cache trees\n", _cacheTrees.size());
	_tree->printTree();
} // CacheOptimizedRenderer::print