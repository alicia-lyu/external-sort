#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (RowSize recordSize, vector<string> runFileNames, u_int32_t pageSize, u_int64_t memorySpace, u_int16_t rendererNumber) :  // 500 KB = 2^19
    SortedRecordRenderer(recordSize, _getOutputFileName()), _pageSize (pageSize), _inputBufferCount (memorySpace / pageSize - 1 - _readAheadBufferCount), _pass (1), _rendererNumber (rendererNumber)
{
    traceprintf("Renderer %d, merging %zu run files with %hu input buffers\n", _rendererNumber, runFileNames.size(), _inputBufferCount);
    // Multi-pass merge
    while (runFileNames.size() > _inputBufferCount) {
        u_int16_t rendererCount = 0;
        _pass++;
        traceprintf("Pass %hhu\n", _pass);
        vector<string> mergedRunNames;
        for (int i = 0; i < runFileNames.size(); i += _inputBufferCount) {
            rendererCount++;
            vector<string> subRunFileNames(
                runFileNames.begin() + i,
                runFileNames.begin() + std::min(i + _inputBufferCount, (int) runFileNames.size())
            );
            // print all subRunFileNames
            for (auto subRunFileName : subRunFileNames) {
                traceprintf("Sub-run file %s\n", subRunFileName.c_str());
            }
            ExternalRenderer * renderer = new ExternalRenderer(recordSize, subRunFileNames, pageSize, memorySpace, rendererCount); // subRunFileNames will be copied
            mergedRunNames.push_back(renderer->run()); // TODO: Graceful degradation
            delete renderer;
        }
        runFileNames = mergedRunNames;
    }
    // Ready to render sorted records
    vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, _pageSize, _recordSize);
        _runs.push_back(run);
        formingRows.push_back(run->next());
    }
    _tree = new TournamentTree(formingRows, _recordSize);
	// this->print();
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
    byte * rendered = _tree->peekRoot();
    if (rendered == nullptr) return nullptr;
    // Copy root before calling run.next()
    // For retrieving a new page will overwrite the current page, where root is in
    byte * output = _addRowToOutputBuffer(rendered);
    // Resume the tournament
	u_int16_t bufferNum = _tree->peekTopBuffer();
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

string ExternalRenderer::_getOutputFileName ()
{
    string topDir;
    return string(".") + SEPARATOR + string("spills") + SEPARATOR + string("pass") +std::to_string(_pass) + SEPARATOR + string("run") + std::to_string(_rendererNumber) + string(".bin");
} // ExternalRenderer::getOutputFileName

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print