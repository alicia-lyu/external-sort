#include "ExternalRenderer.h"
#include "utils.h"

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