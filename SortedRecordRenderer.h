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
	SortedRecordRenderer (RowSize recordSize);
	virtual ~SortedRecordRenderer ();
	virtual byte * next () = 0;
    string run(); // Render all sorted records and store to a file, return the file name
protected:
    Buffer * _outputBuffer;
    ofstream _outputFile;
    RowSize _recordSize;
    virtual string _getOutputFileName() = 0;
    void _addRowToOutputBuffer(byte * row);
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
    u_int16_t _runNumber;
    string _getOutputFileName();
};

class CacheOptimizedRenderer : public SortedRecordRenderer
{
public:
    CacheOptimizedRenderer (RowSize recordSize, vector<TournamentTree *> &cacheTrees, u_int16_t runNumber = 0); // will modify the given cacheTrees, but it won't be reused
    ~CacheOptimizedRenderer ();
    byte * next();
    void print();
private:
    RowSize _recordSize;
    u_int16_t _runNumber;
    TournamentTree * _tree;
    vector<TournamentTree *> _cacheTrees;
    string _getOutputFileName();
};