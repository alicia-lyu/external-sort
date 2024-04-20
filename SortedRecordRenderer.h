#pragma once

#include "TournamentTree.h"
#include "params.h"
#include "Metrics.h"
#include <vector>
#include <fstream>

using std::vector;
using std::ofstream;

class SortedRecordRenderer
{
public:
	SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow = nullptr);
	virtual ~SortedRecordRenderer ();
	virtual byte * next () = 0;
    string run(); // Render all sorted records and store to a file, return the file name
protected:
    Buffer * _outputBuffer;
    ofstream _outputFile;
    RowSize _recordSize;
    string _outputFileName;
    u_int16_t _runNumber;
    u_int64_t _produced;
    bool _removeDuplicates;
    byte * _lastRow;
    string _getOutputFileName(u_int8_t pass, u_int16_t runNumber);
    byte * _addRowToOutputBuffer(byte * row); // return pointer to the row in output buffer
};