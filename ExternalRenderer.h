#pragma once

#include "SortedRecordRenderer.h"
#include "ExternalRun.h"
#include "TournamentTree.h"
#include <vector>
#include <set>

using std::string;
using std::vector;
using std::ofstream;
using std::set;

class ExternalRenderer : public SortedRecordRenderer
{
public:
    ExternalRenderer (RowSize recordSize, 
        vector<string> runFileNames, u_int64_t readAheadSize,
        u_int8_t pass, u_int16_t rendererNumber = 0, 
        bool removeDuplicates = false); // this function will modify runFileNames, so it is passed by value
    ~ExternalRenderer ();
    byte * next();
    void print();
private:
    vector<ExternalRun *> _runs; // 497 runs (1 input buffer), 1 output buffer, 2 read-ahead buffers
    // SSD fan-in F = M/P, SSD passes log_F(I/M). I = 10 GB, M = 100 MB, P = 20 KB, only needs one pass
    // HDD I = 120 GB, M = 100 MB, P = 500 KB, needs 2 passes
    TournamentTree * _tree;
    ExternalRun * longRun;
};