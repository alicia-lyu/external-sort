#include "GracefulRenderer.h"
#include "utils.h"

GracefulRenderer::GracefulRenderer (RowSize recordSize, SortedRecordRenderer * inMemoryRenderer, ExternalRun * externalRun, bool removeDuplicates) :
    SortedRecordRenderer(recordSize, 1, 0, removeDuplicates),
    externalRun(externalRun),
    inMemoryRenderer(inMemoryRenderer)
{
    TRACE (false);
    vector<byte *> formingRows;
    formingRows.push_back(inMemoryRenderer->next());
    formingRows.push_back(externalRun->next());
    tree = new TournamentTree(formingRows.cbegin(), recordSize, formingRows.size());
}

GracefulRenderer::~GracefulRenderer ()
{
    TRACE (false);
    delete externalRun;
    delete inMemoryRenderer;
    delete tree;
}

byte * GracefulRenderer::next ()
{
    return SortedRecordRenderer::renderRow(
        [this] () -> byte * {
            auto bufferNum = tree->peekTopBuffer();
            if (bufferNum == 0) return inMemoryRenderer->next();
            else return externalRun->next();
        },
        tree
    );
}