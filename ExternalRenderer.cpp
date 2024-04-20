#include "ExternalRenderer.h"
#include "utils.h"

ExternalRenderer::ExternalRenderer (RowSize recordSize, vector<string> runFileNames, u_int8_t pass, u_int16_t rendererNumber, bool removeDuplicates) :  // 500 KB = 2^19
    SortedRecordRenderer(recordSize, pass, rendererNumber, removeDuplicates), _pass (pass), _readAheadBuffers (READ_AHEAD_BUFFERS_MIN)
{
    u_int16_t inputBufferCount = MEMORY_SIZE / SSD_PAGE_SIZE - 1 - READ_AHEAD_BUFFERS_MIN;
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
    _readAheadBuffers = std::max(_readAheadBuffers, (u_int16_t) (inputBufferCount - runFileNames.size())); // If there are not enough run files, use the remaining buffers for read-ahead
    double _readAheadThreshold = std::max(1.0 - (double)_readAheadBuffers / runFileNames.size(), 0.5);
    #if defined(VERBOSEL1) || defined(VERBOSEL2)
    traceprintf("Read-ahead buffers: %hu, threshold: %f\n", _readAheadBuffers, _readAheadThreshold);
    #endif
    vector<byte *> formingRows;
    for (auto runFileName : runFileNames) {
        ExternalRun * run = new ExternalRun(runFileName, SSD_PAGE_SIZE, _recordSize, _readAheadBuffers, _readAheadThreshold);
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