#include "InMemoryRenderer.h"
#include "utils.h"

NaiveRenderer::NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber, bool removeDuplicates) :
	SortedRecordRenderer(recordSize, 0, runNumber, removeDuplicates), _tree(tree)
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
	byte * rendered = renderRow(
		[] () -> byte * {
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
	TRACE (false);
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