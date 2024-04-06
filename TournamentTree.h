#include "defs.h"
#include <vector>
#include "Data.h"
#include <tuple>

class Node
{
public:
    std::vector<byte> data;
    Node * left;
    Node * right;
    Node * parent;
    u_int8_t bufferNum;
    Node * farthestLoser;
    Node (std::vector<byte> data, u_int8_t bufferNum, Node * farthestLoser);
    ~Node ();
private:
};

class TournamentTree
{
public:
    TournamentTree (std::vector<byte *> records, RowSize recordSize); // records.size() u_int8_t max 256 records (min 5 MB, larger than 1 MB cache line)
    ~TournamentTree ();
    void push (byte * record);
    u_int8_t poll (byte * outputOffset); // returning from which buffer to fetch the next record (max. 2^8 = 256 buffers)
    void printTree ();
private:
    Node * _root;
    RowSize _recordSize;
    std::tuple<Node *, Node *> _formRoot (std::vector<byte *> records, u_int8_t offset, u_int8_t numRecords);
    std::tuple<Node *, Node *> _contest(Node * root_left, Node * root_right);
    std::vector<byte> _getData(std::vector<byte *> records, u_int8_t offset);
    void _printNode (Node * node, string prefix, bool isLeft);
    void _printRoot ();
};