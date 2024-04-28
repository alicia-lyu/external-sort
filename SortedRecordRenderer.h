#pragma once

#include "TournamentTree.h"
#include "params.h"
#include "Metrics.h"
#include "ExternalRun.h"
#include <vector>
#include <fstream>
#include <functional>

using std::vector;
using std::ofstream;

class Materializer; // Forward declaration

class SortedRecordRenderer
{
    friend class Materializer;
public:
	SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber, bool removeDuplicates, byte * lastRow = nullptr, bool materialize = true);
	virtual ~SortedRecordRenderer ();
	virtual byte * next () = 0;
    string run(); // Render all sorted records and store to a file, return the file name
protected:
    RowSize const _recordSize;
    u_int64_t _produced;
    bool const _removeDuplicates;
    byte * _lastRow;
    byte * renderRow(std::function<byte *()> retrieveNext, TournamentTree * & tree, ExternalRun * longRun = nullptr);
private:
    Materializer * materializer;
};

class Materializer // Materialize the sorted records to a file
{
    friend class SortedRecordRenderer;
public:
    Materializer (u_int8_t pass, u_int16_t runNumber, SortedRecordRenderer * renderer);
    ~Materializer ();
private:
    SortedRecordRenderer * renderer;
    Buffer * outputBuffer;
    ofstream outputFile;
    u_int8_t deviceType;
    string outputFileName;
    string getOutputFileName(u_int8_t pass, u_int16_t runNumber);
    bool flushOutputBuffer(u_int32_t sizeFilled); // max. 500 KB = 2^19
    int switchDevice(u_int32_t sizeFilled);
    byte * addRowToOutputBuffer(byte * row); // return pointer to the row in output buffer
};