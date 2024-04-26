#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (RowSize recordSize, 
    vector<string> runFileNames, u_int64_t readAheadSize,
    u_int8_t pass, u_int16_t rendererNumber, 
    bool removeDuplicates) :  // 500 KB = 2^19
    SortedRecordRenderer(recordSize, pass, rendererNumber, removeDuplicates)
{
    TRACE (false);
    vector<byte *> formingRows;
    ExternalRun::READ_AHEAD_SIZE = readAheadSize;
    ExternalRun::READ_AHEAD_THRESHOLD = std::max(0.5, ((double) readAheadSize) / MEMORY_SIZE);
    #if defined(VERBOSEL1) || defined(VERBOSEL2)
    traceprintf ("Renderer %d: %zu run files, read-ahead size %llu threshold %f\n", rendererNumber, runFileNames.size(), readAheadSize, ExternalRun::READ_AHEAD_THRESHOLD);
    #endif
    for (auto &runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, _recordSize);
        _runs.push_back(run);
        formingRows.push_back(run->next());
    }
    _tree = new TournamentTree(formingRows, _recordSize);
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (false);
    for (auto &run : _runs) {
        delete run;
    }
} // ExternalRenderer::~ExternalRenderer

byte * ExternalRenderer::next ()
{
    TRACE (false);
    return SortedRecordRenderer::renderRow(
		[this] () -> byte * {
			auto bufferNum = _tree->peekTopBuffer();
			auto run = _runs.at(bufferNum);
			return run->next();
		},
		_tree
	);
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print