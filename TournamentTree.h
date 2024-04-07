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
    void inPlaceSort(); // Needed for in-cache sorting, similar to heap sort
    u_int8_t peek (byte * outputOffset); // Needed for merge-sort (any level). Returning from which buffer to fetch the next record (max. 2^8 = 256 buffers)
    void pushAndPoll (byte * record); // Needed for merge-sort (any level). If there are already sorted runs, expect a record from a certain buffer (readable from calling peek first)
    void printTree ();
private:
    u_int8_t _nextNodeToPush; // farthestLoser of the popped
    Node * _root;
    RowSize _recordSize;
    std::tuple<Node *, Node *> _formRoot (std::vector<byte *> records, u_int8_t offset, u_int8_t numRecords);
    std::tuple<Node *, Node *> _contest(Node * root_left, Node * root_right);
    std::vector<byte> _getData(std::vector<byte *> records, u_int8_t offset);
    void _printNode (Node * node, string prefix, bool isLeft);
    void _printRoot ();
    void _checkParents ();
};