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

class ExternalRun
{
public:
    Buffer * inMemoryPage;
    ExternalRun (std::string runFileName, u_int32_t pageSize, RowSize recordSize);
    ~ExternalRun ();
    byte * next();
private:
    std::string _runFileName;
    std::ifstream _runFile;
    u_int32_t _read;
    u_int32_t _pageSize;
};

class ExternalRenderer : public SortedRecordRenderer
{
public:
    ExternalRenderer (std::vector<string> runFileNames, RowSize recordSize, u_int32_t pageSize);
    ~ExternalRenderer ();
    byte * next();
    void print();
private:
    std::vector<ExternalRun *> _runs;
    RowSize _recordSize;
    u_int32_t _pageSize;
    TournamentTree * _tree;
};