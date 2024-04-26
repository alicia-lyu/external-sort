#include "SortedRecordRenderer.h"

class NaiveRenderer : public SortedRecordRenderer
{
public:
    NaiveRenderer (RowSize recordSize, TournamentTree * tree, u_int16_t runNumber = 0, bool removeDuplicates = false, bool materialize = true); // max. 120 G / 100 M = 2^10
    ~NaiveRenderer ();
    byte * next ();
    void print();
private:
    TournamentTree * _tree;
};

class CacheOptimizedRenderer : public SortedRecordRenderer
{
public:
    CacheOptimizedRenderer (RowSize recordSize, vector<TournamentTree *> &cacheTrees, u_int16_t runNumber = 0, bool removeDuplicates = false, bool materialize = true); // will modify the given cacheTrees, but it won't be reused
    ~CacheOptimizedRenderer ();
    byte * next();
    void print();
private:
    TournamentTree * _tree;
    vector<TournamentTree *> _cacheTrees;
};