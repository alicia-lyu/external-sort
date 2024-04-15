#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize, u_int64_t memorySpace, u_int16_t rendererNumber) :  // 500 KB = 2^19
    _recordSize (recordSize), _pageSize (pageSize), _inputBufferCount (memorySpace / pageSize - 1 - _readAheadBufferCount), _pass (1), _rendererNumber (rendererNumber)
{
    traceprintf("Renderer %d in pass %d, merging %zu run files with %d input buffers\n", _rendererNumber, _pass, runFileNames.size(), _inputBufferCount);
    // Multi-pass merge
    while (runFileNames.size() > _inputBufferCount) {
        int rendererCount = 0;
        _pass++;
        traceprintf("Pass %d\n", _pass);
        std::vector<string> mergedRunNames;
        for (int i = 0; i < runFileNames.size(); i += _inputBufferCount) {
            rendererCount++;
            std::vector<string> subRunFileNames(
                runFileNames.begin() + i, 
                runFileNames.begin() + std::min(i + _inputBufferCount, (int) runFileNames.size())
            );
            ExternalRenderer * renderer = new ExternalRenderer(subRunFileNames, recordSize, pageSize, memorySpace, rendererCount);
            mergedRunNames.push_back(renderer->run());
            delete renderer;
        }
        runFileNames = mergedRunNames;
    }
    // Ready to render sorted records
    std::vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, _pageSize, _recordSize);
        _runs.push_back(run);
        formingRows.push_back(run->next());
    }
    _tree = new TournamentTree(formingRows, _recordSize);
    outputBuffer = new Buffer(_pageSize / _recordSize, _recordSize);
	// this->print();
} // ExternalRenderer::ExternalRenderer

ExternalRenderer::~ExternalRenderer ()
{
	TRACE (true);
    _writeOutputToDisk(outputBuffer->sizeFilled());
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
        // traceprintf("Output buffer is full, write to disk\n"); 
        _writeOutputToDisk(_pageSize);
        output = outputBuffer->copy(rendered);
    }
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

string ExternalRenderer::run ()
{
    TRACE (true);
    byte * row = next();
    while (row != nullptr) {
        row = next();
    }
    return _getOutputFileName();
} // ExternalRenderer::run

string ExternalRenderer::_getOutputFileName ()
{
    return std::string(".") + SEPARATOR + std::string("outputs") + SEPARATOR + std::to_string(_pass) + SEPARATOR + std::to_string(_rendererNumber) + std::string("output.bin");
} // ExternalRenderer::getOutputFileName

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print

void ExternalRenderer::_writeOutputToDisk (u_int32_t writeSize)
{
    // TODO: split to multiple output files
    std::ofstream outputFile(_getOutputFileName(), std::ios::binary | std::ios::app);
    outputFile.write((char *) outputBuffer->data(), writeSize);
    outputFile.close();
} // ExternalRenderer::writeOutputToDisk