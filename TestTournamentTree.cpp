#include "Data.h"
#include "TournamentTree.h"
#include "utils.h"
#include <memory>
#include <vector>

int main (int argc, char * argv [])
{
    RowSize recordSize = 20;
    u_int8_t recordCount = 10;
    MemoryRun * run = new MemoryRun(recordCount, recordSize);
    std::vector<byte *> records;
    for (u_int8_t i = 0; i < recordCount; i++) {
        run->fillRowRandomly(i);
        records.push_back(run->getRow(i));
    }
    TournamentTree * tree = new TournamentTree(records, recordSize);
    tree->printTree();
    byte * outputBuffer = (byte *) malloc(recordCount * recordSize * sizeof(byte));
    u_int8_t bufferNum = tree->peek(outputBuffer);
    printf("Peeked %d\n", bufferNum);
    byte * newRecord = (byte *) malloc(recordSize * sizeof(byte));
    std::copy(records.front(), records.front() + recordSize, newRecord);
    tree->pushAndPoll(newRecord);
    tree->printTree();
    delete tree;
    free(newRecord);
    free(outputBuffer);
    delete run;
}