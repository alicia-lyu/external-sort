#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (RowSize recordSize, 
    vector<string> runFileNames, u_int64_t readAheadSize,
    u_int8_t pass, u_int16_t rendererNumber, 
    bool removeDuplicates) :  // 500 KB = 2^19
    SortedRecordRenderer(recordSize, pass, rendererNumber, removeDuplicates), _pass (pass)
{
    #if defined(VERBOSEL1) || defined(VERBOSEL2)
    traceprintf ("Renderer %d: %zu run files, read-ahead size %llu\n", rendererNumber, runFileNames.size(), readAheadSize);
    #endif
    #if defined(VERBOSEL2)
    for (auto runName : runFileNames) {
        traceprintf ("%s\n", runName.c_str());
    }
    #endif
    vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, _recordSize, readAheadSize);
        _runs.push_back(run);
        formingRows.push_back(run->next());
    }
    _tree = new TournamentTree(formingRows, _recordSize);
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (false);
    for (auto run : _runs) {
        delete run;
    }
} // ExternalRenderer::~ExternalRenderer

byte * ExternalRenderer::next ()
{
    TRACE (false);
    byte * rendered = renderRow(
		[this] () -> byte * {
			auto bufferNum = _tree->peekTopBuffer();
			auto run = _runs.at(bufferNum);
			return run->next();
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
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print