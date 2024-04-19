#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (RowSize recordSize, vector<string> runFileNames, u_int8_t pass, u_int16_t rendererNumber) :  // 500 KB = 2^19
    SortedRecordRenderer(recordSize, pass, rendererNumber), _pass (pass)
{
    u_int16_t inputBufferCount = MEMORY_SIZE / SSD_PAGE_SIZE - 1 - READ_AHEAD_BUFFERS;
    traceprintf("Pass %hhu Renderer %d, merging %zu run files with %hu input buffers\n", _pass, _runNumber, runFileNames.size(), inputBufferCount);
    
    // Incur one pass above
    if (runFileNames.size() > inputBufferCount) {
        u_int16_t rendererCount = 0;
        _pass++;
        int runCountNextPass = (runFileNames.size() + inputBufferCount - 1) / inputBufferCount; 
        // if still too many runs, will be merged in the constructors next pass
        vector<string> mergedRunNames;
        for (int i = 0; i < runFileNames.size(); i += runCountNextPass) {
            rendererCount++;
            vector<string> subRunFileNames(
                runFileNames.begin() + i,
                runFileNames.begin() + std::min(i + runCountNextPass, (int) runFileNames.size())
            );
            #ifdef VERBOSEL2
            for (auto subRunFileName : subRunFileNames) {
                traceprintf("Sub-run file %s\n", subRunFileName.c_str());
            }
            #endif
            ExternalRenderer * renderer = new ExternalRenderer(recordSize, subRunFileNames, pass - 1, rendererCount); // subRunFileNames will be copied
            mergedRunNames.push_back(renderer->run()); // TODO: Graceful degradation
            delete renderer;
        }
        runFileNames = mergedRunNames;
    }

    // Ready to render sorted records
    vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, SSD_PAGE_SIZE, _recordSize);
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
    #ifdef VERBOSEL2
    traceprintf("Produced %s\n", rowToString(output, _recordSize).c_str());
    #endif
    return output;
} // ExternalRenderer::next

void ExternalRenderer::print ()
{
    traceprintf ("%zu run files\n", _runs.size());
	_tree->printTree();
} // ExternalRenderer::print