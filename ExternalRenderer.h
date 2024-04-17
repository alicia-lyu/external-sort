#include "SortedRecordRenderer.h"
#include "ExternalRun.h"
#include "TournamentTree.h"
#include <vector>

using std::string;
using std::vector;
using std::ofstream;

class ExternalRenderer : public SortedRecordRenderer
{
public:
    Buffer * outputBuffer;
    ExternalRenderer (vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize, u_int64_t memorySpace, u_int16_t rendererNumber = 0); // this function will modify runFileNames, so it is passed by value
    ~ExternalRenderer ();
    byte * next();
    string run();
    void print();
private:
    u_int8_t const _readAheadBufferCount = 0; // TODO: Implement read-ahead buffers
    vector<ExternalRun *> _runs; // 497 runs (1 input buffer), 1 output buffer, 2 read-ahead buffers
    RowSize const _recordSize;
    u_int32_t const _pageSize;
    u_int16_t const _inputBufferCount; // max. 100 MB / 20 KB = 5000 = 2^13
    u_int8_t _pass;
    u_int16_t const _rendererNumber; // # of the renderer in the same pass
    // SSD fan-in F = M/P, SSD passes log_F(I/M). I = 10 GB, M = 100 MB, P = 20 KB, only needs one pass
    // HDD I = 120 GB, M = 100 MB, P = 500 KB, needs 2 passes
    TournamentTree * _tree;
    ofstream _outputFile;
    string _getOutputFileName();
};