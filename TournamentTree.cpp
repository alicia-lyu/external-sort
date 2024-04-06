#include "TournamentTree.h"
#include <queue>

Node::Node (std::vector<byte> data, u_int8_t bufferNum, Node * farthestLoser)
: data (data), left (nullptr), right (nullptr),parent (nullptr), 
    bufferNum (bufferNum), farthestLoser (farthestLoser)
{   
    TRACE (true);
}

Node::~Node ()
{
    TRACE (true);
}

TournamentTree::TournamentTree (std::vector<byte *> records, RowSize record_size)
{
    TRACE (true);
    _formRoot (records, 0, records.size());
}

TournamentTree::~TournamentTree ()
{
    TRACE(true);
    std::queue<Node *> q; 
    Node * node = _root;
    while (node != nullptr) {
        q.push(node->left);
        q.push(node->right);
        delete node;
        node = q.front();
        q.pop();
    }
}

std::tuple<Node *, Node *>  TournamentTree::_formRoot (std::vector<byte *> records, u_int8_t offset, u_int8_t numRecords)
{
    if (numRecords == 1) {
        Node * root = new Node(_getData(records, offset), offset, nullptr);
        return std::make_tuple(root, nullptr);
    }
    Node * left_winner;
    Node * right_winner;
    Node * root;
    Node * loser;
    if (numRecords == 2) {
        left_winner = new Node(_getData(records, offset), offset, nullptr);
        right_winner = new Node(_getData(records, offset+1), offset, nullptr);
        std::tie(root, loser) = _contest(left_winner, right_winner);
    } else {
        u_int8_t numRecordsLeft = 2;
        while (numRecordsLeft < numRecords) {
            numRecordsLeft *= 2;
        }
        numRecordsLeft /= 2;
        Node * left_loser;
        Node * right_loser;
        std::tie(left_winner, left_loser) = _formRoot(records, offset, numRecordsLeft);
        std::tie(right_winner, right_loser) = _formRoot(records, offset + numRecordsLeft, numRecords - numRecordsLeft);
        std::tie(root, loser) = _contest(left_winner, right_winner);
        if (left_loser != nullptr) left_loser->parent = loser;
        if (right_loser != nullptr) right_loser->parent = loser;
    }
    return std::make_tuple(root, loser);
}

std::tuple<Node *, Node *> TournamentTree::_contest (Node * root_left, Node * root_right)
{
    Node * winner;
    Node * loser;
    if (root_left->data <= root_right->data) {
        winner = root_left;
        loser = root_right;
    } else {
        winner = root_right;
        loser = root_left;
    }
    if (winner->farthestLoser == nullptr) {
        winner->farthestLoser = loser;
    }
    loser->parent = winner;
    return std::make_tuple(winner, loser);
}