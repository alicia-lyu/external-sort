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
    RowSize _recordSize;
    string _outputFileName;
    u_int16_t _runNumber;
    u_int64_t _produced;
    bool _removeDuplicates;
    byte * _lastRow;
    string _getOutputFileName(u_int8_t pass, u_int16_t runNumber);
    byte * _addRowToOutputBuffer(byte * row); // return pointer to the row in output buffer
private:
    Buffer * _outputBuffer;
    ofstream _outputFile;
    void _write(u_int16_t sizeFilled); // max. 500 KB / 20 B = 25000 = 2^14
};