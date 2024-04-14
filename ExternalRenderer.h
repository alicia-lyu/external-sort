#include "SortedRecordRenderer.h"
#include "ExternalRun.h"
#include "TournamentTree.h"
#include <vector>

class ExternalRenderer : public SortedRecordRenderer
{
public:
    Buffer * outputBuffer;
    ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize);
    ~ExternalRenderer ();
    byte * next();
    void print();
private:
    std::vector<ExternalRun *> _runs;
    RowSize _recordSize;
    u_int32_t _pageSize;
    TournamentTree * _tree;
    u_int32_t _outputCount; // 10 GB / 20 KB = 2^19; 120 GB / 500 KB = 2^19
    void _writeOutputToDisk(u_int32_t writeSize);
};