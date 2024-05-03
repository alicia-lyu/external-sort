#pragma once

#include "defs.h"
#include <vector>
#include "Buffer.h"
#include <tuple>
#include <cstring>

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
    Node (byte * &data, RowSize size, u_int16_t bufferNum, Node * farthestLoser);
    ~Node ();
private:
};

class TournamentTree
{
public:
    TournamentTree (vector<byte *>::const_iterator records, u_int16_t recordSize, u_int16_t numRecords);
    ~TournamentTree ();
    byte * poll (); // When the buffer that the root comes from is exhausted, advance the farthest loser of the root, push the tree all the way to the top, and poll the root
    u_int16_t peekTopBuffer (); // Peek the buffer number of the root
    byte * peekRoot(); // Peek the data stored in root node
    byte * pushAndPoll (byte * record); // When the buffer that the root comes from is not exhausted, use a new record therefrom to push the tree all the way up to poll the root
    void printTree ();
private:
    Node * _root;
    RowSize _recordSize;
    tuple<Node *, Node *> _formRoot (vector<byte *>::const_iterator &records, u_int16_t offset, u_int16_t numRecords); // max records.size() = 100 MB / 20 KB = 2^13
    tuple<Node *, Node *> _contest(Node * root_left, Node * root_right);
    Node * _advanceToTop(Node * advancing, Node * incumbent);
    void _printNode (Node * node, string prefix, bool isLeft);
    void _printRoot ();
    void _checkParents ();
};