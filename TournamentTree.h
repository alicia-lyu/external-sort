#pragma once

#include "defs.h"
#include <vector>
#include "Buffer.h"
#include <tuple>

using std::vector;
using std::tuple;

class Node
{
public:
    byte * data;
    RowSize _size;
    Node * left;
    Node * right;
    Node * parent;
    u_int16_t bufferNum;
    Node * farthestLoser;
    Node (byte * data, RowSize size, u_int16_t bufferNum, Node * farthestLoser);
    ~Node ();
private:
};

class TournamentTree
{
public:
    TournamentTree (const vector<byte *> &records, RowSize recordSize);
    ~TournamentTree ();
    byte * poll (); // Needed for in-memory sorting, or when there are no more records to be pushed while polling
    u_int16_t peekTopBuffer (); // Works with pushAndPoll for merge-sort. Returning from which buffer to fetch the next record (max. 2^8 = 256 buffers)
    byte * peekRoot();
    byte * pushAndPoll (byte * record); // Needed for merge-sort. Expect a record from a certain buffer (readable from calling peek first)
    void printTree ();
private:
    Node * _root;
    RowSize _recordSize;
    tuple<Node *, Node *> _formRoot (const vector<byte *> &records, u_int16_t offset, u_int16_t numRecords);
    // max records.size() = 100 MB / 20 KB = 2^13
    tuple<Node *, Node *> _contest(Node * root_left, Node * root_right);
    Node * _advanceToTop(Node * advancing, Node * incumbent); // Called when _root is going to be polled, returns the previous root
    void _printNode (Node * node, string prefix, bool isLeft);
    void _printRoot ();
    void _checkParents ();
};