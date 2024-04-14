#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize) :  // 500 KB = 2^19
    _recordSize (recordSize), _pageSize (pageSize), _outputCount (0)
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
        traceprintf("#%d Output buffer is full, write to disk\n", _outputCount); 
        _writeOutputToDisk(_pageSize);
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

void ExternalRenderer::_writeOutputToDisk (u_int32_t writeSize)
{
    string outputFileName = std::string(".") + SEPARATOR + std::string("outputs") + SEPARATOR + std::to_string(_outputCount++) + std::string(".bin"); 
    // TODO: In multi-level merge, need to spill intermediate results to disk
    std::ofstream outputFile(outputFileName, std::ios::binary);
    outputFile.write((char *) outputBuffer->data(), writeSize);
    outputFile.close();
} // ExternalRenderer::writeOutputToDisk