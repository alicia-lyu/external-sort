#include "TournamentTree.h"
#include <queue>
#include <iostream>
#include "utils.h"
#include <utility>
#include <algorithm>

Node::Node (byte * data, RowSize size, u_int8_t bufferNum, Node * farthestLoser)
: data (data), _size (size), left (nullptr), right (nullptr),parent (nullptr), 
    bufferNum (bufferNum), farthestLoser (farthestLoser)
{   
    TRACE (false);
    string recordContent = rowToHexString(data, _size);
    // traceprintf("Node created with bufferNum %d and data %s\n", bufferNum, recordContent.c_str());
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
    // TRACE (true);
    if (numRecords == 1) {
        Node * root = new Node(records.at(offset), _recordSize, offset, nullptr);
        return std::make_tuple(root, nullptr);
    }
    Node * left_winner;
    Node * right_winner;
    Node * root;
    Node * loser;
    if (numRecords == 2) {
        left_winner = new Node(records.at(offset), _recordSize,  offset, nullptr);
        right_winner = new Node(records.at(offset+1), _recordSize, offset + 1, nullptr);
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
        // Take care of the pointers involving two losers (two leaves)
        if (left_loser != nullptr) left_loser->parent = loser;
        loser->left = left_loser;
        if (right_loser != nullptr) right_loser->parent = loser;
        loser->right = right_loser;
    }
    // Take care of the pointers not involving two losers (the stem of root and second)
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
    // traceprintf("Winner %d, loser %d\n", winner->bufferNum, loser->bufferNum);
    return std::make_tuple(winner, loser);
}

Node * TournamentTree::_advanceToTop(Node * advancing, Node * incumbent)
{
    Node * winner;
    Node * loser;
    Node * lastLoser = nullptr;
    bool incumbentIsLeft;
    traceprintf("Advancing %d, incumbent %d\n", advancing->bufferNum, incumbent->bufferNum);
    while (incumbent != _root) { // Guaranteed that incumbent arg is not root
        // Stop at root, as root is going to be polled -- it is already part of the history
        // Advancing cannot be larger than root; it is guaranteed by the logic outside tournament tree -- merge sort
        std::tie(winner, loser) = _contest(incumbent, advancing);
        // Loser inherit all pointers from incumbent---Nodes settle when they lose
        //// Update the parent of the last loser
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
        //// Update left and right children of loser
        } else {
           leftChild = incumbent->left;
           rightChild = incumbent->right;
        }
        loser->left = leftChild;
        if (leftChild != nullptr) leftChild->parent = loser;
        loser->right = rightChild;
        if (rightChild != nullptr) rightChild->parent = loser;
        // Winner keeps advancing, no need to change pointers, as they will be overwritten
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
    Node * previousRoot = _root;
    _root = advancing;
    return previousRoot;
}

void TournamentTree::inPlaceSort()
{
    TRACE (true);
    // TODO
}

byte * TournamentTree::poll()
{
    // TRACE (true);
    if (_root == nullptr) {
        return nullptr;
    }
    Node * advancing = _root->farthestLoser;
    Node * previousRoot;
    if (advancing == nullptr) {
        previousRoot = _root;
        _root = nullptr;
    } else {
        Node * incumbent = advancing->parent; // advancing, if exists, cannot be root
        if (incumbent == _root) {
            previousRoot = _root;
            _root = advancing;
            advancing->parent = nullptr;
        } else {
            // Advancing breaks tie to become a free node
            Node * child;
            if (advancing->left != nullptr && advancing->right != nullptr) {
                traceprintf("Advancing %d has two children %d and %d\n", advancing->bufferNum, advancing->left->bufferNum, advancing->right->bufferNum);
                // Farthest loser of root should have reached further down the tree.
                exit(1);
            } else if (advancing->left != nullptr) {
                child = advancing->left;
            } else {
                child = advancing->right;
            }
            advancing->parent = nullptr;
            if (incumbent->left == advancing) incumbent->left = child;
            else incumbent->right = child;
            previousRoot = _advanceToTop(advancing, incumbent);
        }
    }
    byte * polled = previousRoot->data;
    traceprintf("Polled %d\n", previousRoot->bufferNum);
    delete previousRoot;
    return polled;
}

u_int8_t TournamentTree::peek()
{
    TRACE (true);
    return _root->bufferNum;
}

byte * TournamentTree::pushAndPoll(byte * record)
{
    TRACE (true);
    Node * advancing = new Node(record, _recordSize, _root->bufferNum, nullptr);
    // The new record is intended to come from the same buffer as the root that is going to be popped
    Node * incumbent = _root->farthestLoser; 
    // farthest loser will never be a Node that is popped (it is a descendant of whatever Node)
    Node * previousRoot = _advanceToTop(advancing, incumbent);
    // Poll (advancing farthestLoser) and push (advancing new record) separately do not work.
    // As we are only polling one record, we cannot advance twice. A node never retreats to
    // lower levels, but only advances.
    byte * polled = previousRoot->data;
    traceprintf("Pushed %d and polled %d\n", advancing->bufferNum, previousRoot->bufferNum);
    delete previousRoot;
    return polled;
}

void TournamentTree::printTree ()
{
    TRACE (true);
    if (_root == nullptr) {
        printf("Empty tree\n");
        return;
    }
    _checkParents();
    _printRoot();
    _printNode(_root, "", true);
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
    if (_root->farthestLoser == nullptr) {
        printf("Root: buffer %d, farthest loser NULL\n", _root->bufferNum);
    } else {
        printf("Root: buffer %d, farthest loser %d\n", _root->bufferNum, _root->farthestLoser->bufferNum);
    }
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
                    printf("Parent of left child %d is %d, expected %d\n", node->left->bufferNum, node->left->parent->bufferNum, node->bufferNum);
                    exit(1);
                }
                q.push(node->left);
            }
            if (node->right != nullptr) {
                if (node->right->parent != node) {
                    printf("Parent of right child %d is %d, expected %d\n", node->right->bufferNum, node->right->parent->bufferNum, node->bufferNum);
                    exit(1);
                }
                q.push(node->right);
            }
        }
    }
}