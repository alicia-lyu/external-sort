#pragma once

#include "TournamentTree.h"
#include <vector>

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

class ExternalRenderer : public SortedRecordRenderer
{
public:
    ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize);
    ~ExternalRenderer ();
    byte * next();
    void print();
private:
    std::vector<string> _runFileNames;
    RowSize _recordSize;
    u_int32_t _pageSize;
    int _runCount;
    TournamentTree * _tree;
    std::vector<byte *> _pages;
    std::vector<u_int32_t> _currentPages; // Largest run possible 60 GB, page size 500 KB, pages per run 120000 = 2^17
    std::vector<u_int16_t> _currentRecords; // Largest page possible 500 KB, record size min 20 bytes, records per page 25000 = 2^14
};