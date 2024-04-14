#pragma once

#include "TournamentTree.h"
#include <vector>
#include <fstream>

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
    CacheOptimizedRenderer (std::vector<TournamentTree *> cacheTrees, RowSize recordSize);
    ~CacheOptimizedRenderer ();
    byte * next();
    void print();
private:
    RowSize _recordSize;
    TournamentTree * _tree;
    std::vector<TournamentTree *> _cacheTrees;
};