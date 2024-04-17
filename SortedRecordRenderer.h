#pragma once

#include "TournamentTree.h"
#include <vector>
#include <fstream>

using std::vector;

class SortedRecordRenderer
{
public:
	SortedRecordRenderer ();
	virtual ~SortedRecordRenderer ();
	virtual byte * next () = 0;
};

class NaiveRenderer : public SortedRecordRenderer
{
public:
    NaiveRenderer (TournamentTree * tree);
    ~NaiveRenderer ();
    byte * next ();
    void print();
private:
    TournamentTree * _tree;
};

class CacheOptimizedRenderer : public SortedRecordRenderer
{
public:
    CacheOptimizedRenderer (vector<TournamentTree *> &cacheTrees, RowSize recordSize);
    ~CacheOptimizedRenderer ();
    byte * next();
    void print();
private:
    RowSize _recordSize;
    TournamentTree * _tree;
    vector<TournamentTree *> _cacheTrees;
};