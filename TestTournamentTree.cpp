#include "Data.h"
#include "TournamentTree.h"
#include "utils.h"
#include <memory>
#include <vector>

int main (int argc, char * argv [])
{
    RowSize recordSize = 20;
    u_int8_t recordCount = 10;
    Buffer * buffer = new Buffer(recordCount, recordSize);
    std::vector<byte *> records;
    for (u_int8_t i = 0; i < recordCount; i++) {
        buffer->fillRandomly();
        // records.push_back(buffer->getRow(i));
    }
    TournamentTree * tree = new TournamentTree(records, recordSize);
    tree->printTree();
    u_int8_t bufferNum = tree->peek();
    printf("Peeked %d\n", bufferNum);
    byte * newRecord = (byte *) malloc(recordSize * sizeof(byte));
    std::copy(records.front(), records.front() + recordSize, newRecord);
    tree->pushAndPoll(newRecord);
    tree->printTree();
    for (int i = 0; i < recordCount; i++) {
        tree->poll();
        tree->printTree();
    }
    delete tree;
    free(newRecord);
    delete buffer;
}