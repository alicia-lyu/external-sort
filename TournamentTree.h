#include "defs.h"
#include <vector>
#include "Data.h"
#include <tuple>

class Node
{
public:
    byte * data;
    RowSize _size;
    Node * left;
    Node * right;
    Node * parent;
    u_int8_t bufferNum;
    Node * farthestLoser;
    Node (byte * data, RowSize size, u_int8_t bufferNum, Node * farthestLoser);
    ~Node ();
private:
};

class TournamentTree
{
public:
    TournamentTree (std::vector<byte *> records, RowSize recordSize); // records.size() u_int8_t max 256 records (min 5 MB, larger than 1 MB cache line)
    ~TournamentTree ();
    void inPlaceSort(); // Needed for in-cache sorting
    byte * poll (); // Needed for in-memory sorting, or when there are no more records to be pushed while polling
    u_int8_t peek (); // Works with pushAndPoll for merge-sort. Returning from which buffer to fetch the next record (max. 2^8 = 256 buffers)
    byte * pushAndPoll (byte * record); // Needed for merge-sort. Expect a record from a certain buffer (readable from calling peek first)
    void printTree ();
private:
    u_int8_t _nextNodeToPush; // farthestLoser of the polled
    Node * _root;
    RowSize _recordSize;
    std::tuple<Node *, Node *> _formRoot (std::vector<byte *> records, u_int8_t offset, u_int8_t numRecords);
    std::tuple<Node *, Node *> _contest(Node * root_left, Node * root_right);
    std::vector<byte> _getData(std::vector<byte *> records, u_int8_t offset);
    Node * _advanceToTop(Node * advancing, Node * incumbent); // Called when _root is going to be polled, returns the previous root
    void _printNode (Node * node, string prefix, bool isLeft);
    void _printRoot ();
    void _checkParents ();
};