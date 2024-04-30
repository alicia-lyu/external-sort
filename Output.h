#pragma once

#include "defs.h"
#include "utils.h"
#include "Buffer.h"
#include <fstream>
#include <string>

using std::ifstream;
using std::ofstream;
using std::ios;

class OutputPrinter
{
public:
    OutputPrinter (const string &outputPath, const RowSize recordSize);
    ~OutputPrinter ();
    void Print();
private:
    ifstream answerFile;
    ofstream outputFile;
    RowSize const _recordSize;
    byte * _lastRow;
}; // class OutputPrinter