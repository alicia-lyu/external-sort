#include "TournamentTree.h"
#include <queue>
#include <iostream>
#include "utils.h"
#include <utility>
#include <algorithm>

Node::Node (std::vector<byte> data, u_int8_t bufferNum, Node * farthestLoser)
: data (data), left (nullptr), right (nullptr),parent (nullptr), 
    bufferNum (bufferNum), farthestLoser (farthestLoser)
{   
    TRACE (false);
    string recordContent = rowToHexString(data.data(), data.size());
    traceprintf("Node created with bufferNum %d and data %s\n", bufferNum, recordContent.c_str());
}

Node::~Node ()
{
    TRACE (false);
    // traceprintf("Node deleted with bufferNum %d\n", bufferNum);
}

TournamentTree::TournamentTree (std::vector<byte *> records, RowSize recordSize)
: _recordSize (recordSize)
{
    TRACE (true);
    Node * root;
    Node * second;
    std::tie(root, second) = _formRoot (records, 0, records.size());
    _root = root;
}

TournamentTree::~TournamentTree ()
{
    TRACE(false);
    std::queue<Node *> q;
    q.push(_root);
    Node * node;
    while (q.empty() == false) {
        node = q.front();
        q.pop();
        if (node != nullptr) {
            q.push(node->left);
            q.push(node->right);
            delete node;
        }
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
        right_winner = new Node(_getData(records, offset+1), offset + 1, nullptr);
        std::tie(root, loser) = _contest(left_winner, right_winner);
    } else {
        u_int8_t numRecordsLeft = 2;
        while (numRecordsLeft < numRecords / 2) {
            numRecordsLeft *= 2;
        }
        Node * left_loser;
        Node * right_loser;
        std::tie(left_winner, left_loser) = _formRoot(records, offset, numRecordsLeft);
        std::tie(right_winner, right_loser) = _formRoot(records, offset + numRecordsLeft, numRecords - numRecordsLeft);
        std::tie(root, loser) = _contest(left_winner, right_winner);
        if (left_loser != nullptr) left_loser->parent = loser;
        loser->left = left_loser;
        if (right_loser != nullptr) right_loser->parent = loser;
        loser->right = right_loser;
    }
    loser->parent = root;
    root->left = loser;
    return std::make_tuple(root, loser);
}

std::tuple<Node *, Node *> TournamentTree::_contest (Node * root_left, Node * root_right)
{
    TRACE (false);
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
    traceprintf("Winner %d, loser %d\n", winner->bufferNum, loser->bufferNum);
    return std::make_tuple(winner, loser);
}

std::vector<byte> TournamentTree::_getData(std::vector<byte *> records, u_int8_t offset)
{
    std::vector<byte> data;
    data.assign(records[offset], records[offset] + _recordSize);
    return data;
}

u_int8_t TournamentTree::peek(byte * outputOffset)
{
    TRACE (true);
    std::copy(_root->data.begin(), _root->data.end(), outputOffset);
    return _root->bufferNum;
}

void TournamentTree::pushAndPoll(byte * record)
{
    TRACE (true);
    std::vector<byte> recordData;
    recordData.assign(record, record + _recordSize);
    Node * advancing = new Node(recordData, _root->bufferNum, nullptr);
    Node * incumbent = _root->farthestLoser;
    Node * winner;
    Node * loser;
    Node * lastLoser = nullptr;
    bool incumbentIsLeft;
    while (incumbent != _root) {
        std::tie(winner, loser) = _contest(incumbent, advancing);
        // Loser inherit all pointers from incumbent
        //// Nodes settle when they lose
        Node * leftChild;
        Node * rightChild;
        if (lastLoser != nullptr) {
            if (incumbentIsLeft) {
                leftChild = lastLoser;
                rightChild = incumbent->right;
            } else {
                rightChild = lastLoser;
                leftChild = incumbent->left;
            }
        } else {
           leftChild = incumbent->left;
           rightChild = incumbent->right;
        }
        loser->left = leftChild;
        if (leftChild != nullptr) leftChild->parent = loser;
        loser->right = rightChild;
        if (rightChild != nullptr) rightChild->parent = loser;
        //// The parent of loser is updated next round
        advancing = winner;
        // Record lastLoser and lastIncumbent
        lastLoser = loser;
        incumbentIsLeft = incumbent->parent->left == incumbent; // Will never reach root, so no need to check for nullptr
        // Next incumbent to challenge
        incumbent = incumbent->parent;
    }
    advancing->parent = nullptr;
    advancing->left = loser;
    advancing->right = nullptr;
    loser->parent = advancing;
    delete _root; // popped
    _root = advancing;
}

void TournamentTree::printTree ()
{
    TRACE (true);
    _printRoot();
    _printNode(_root->left, "", true);
    _checkParents();
}

void TournamentTree::_printNode (Node * node, string prefix, bool isLeft)
{
    if (node == nullptr) return;
    _printNode(node->right, prefix + (isLeft ? "│   " : "    "), false);
    printf("%s%s %d\n", prefix.c_str(), (isLeft ? "└── " : "┌──"), node->bufferNum);
    _printNode(node->left, prefix + (isLeft ? "    " : "│   "), true);
}

void TournamentTree::_printRoot ()
{
    printf("Root: buffer %d, farthest loser %d\n", _root->bufferNum, _root->farthestLoser->bufferNum);
}

void TournamentTree::_checkParents ()
{
    std::queue<Node *> q;
    q.push(_root);
    Node * node;
    while (q.empty() == false) {
        node = q.front();
        q.pop();
        if (node != nullptr) {
            if (node->left != nullptr) {
                if (node->left->parent != node) {
                    printf("Parent of left child %d is %d\n", node->left->bufferNum, node->left->parent->bufferNum);
                }
                q.push(node->left);
            }
            if (node->right != nullptr) {
                if (node->right->parent != node) {
                    printf("Parent of right child %d is %d\n", node->right->bufferNum, node->right->parent->bufferNum);
                }
                q.push(node->right);
            }
        }
    }
}