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
    outputBuffer = new Buffer(140 / _recordSize, _recordSize); // TODO: 140 is a magic number
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
    TRACE (true);
    byte * rendered = _tree->peekRoot();
    if (rendered == nullptr) return nullptr;
    // Copy root before calling run.next()
    // For retrieving a new page will overwrite the current page, where root is in
    byte * output = outputBuffer->copy(rendered);
    while (output == nullptr) {
        // Output buffer is full
        // TODO: Write to disk
        output = outputBuffer->copy(rendered);
    }
    // Resume the tournament
	u_int8_t bufferNum = _tree->peekTopBuffer();
    ExternalRun * run = _runs.at(bufferNum);
    byte * retrieved = run->next();
    if (retrieved == nullptr) {
        _tree->poll();
    } else {
        _tree->pushAndPoll(retrieved);
    }
    // traceprintf("Produced %s\n", rowToHexString(output, _recordSize).c_str());
    return output;
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print