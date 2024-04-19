#pragma once

#include "TournamentTree.h"
#include "params.h"
#include <vector>
#include <fstream>

using std::vector;
using std::ofstream;

class SortedRecordRenderer
{
public:
	SortedRecordRenderer (RowSize recordSize, u_int8_t pass, u_int16_t runNumber);
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
    string _getOutputFileName(u_int8_t pass, u_int16_t runNumber);
    byte * _addRowToOutputBuffer(byte * row); // return pointer to the row in output buffer
};

class NaiveRenderer : public SortedRecordRenderer
{
public:
    NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber = 0); // max. 120 G / 100 M = 2^10
    ~NaiveRenderer ();
    byte * next ();
    void print();
private:
    TournamentTree * _tree;
};

class CacheOptimizedRenderer : public SortedRecordRenderer
{
public:
    CacheOptimizedRenderer (RowSize recordSize, vector<TournamentTree *> &cacheTrees, u_int16_t runNumber = 0, bool removeDuplicates = false); // will modify the given cacheTrees, but it won't be reused
    ~CacheOptimizedRenderer ();
    byte * next();
    void print();
private:
    TournamentTree * _tree;
    vector<TournamentTree *> _cacheTrees;
    bool _removeDuplicates;
    byte * lastRow;
    RowCount _removed;
};