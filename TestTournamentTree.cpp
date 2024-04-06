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
        printf("Record %d: %s\n", i, rowToHexString(run->getRow(i), recordSize).c_str());
    }
    TournamentTree * tree = new TournamentTree(records, recordSize);
    tree->printTree();
    delete tree;
}