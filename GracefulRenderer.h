#include "SortedRecordRenderer.h"
#include "ExternalRun.h"

class GracefulRenderer : public SortedRecordRenderer
{
public:
    GracefulRenderer (RowSize recordSize, SortedRecordRenderer * inMemoryRenderer, ExternalRun * externalRun, bool removeDuplicates = false);
    ~GracefulRenderer ();
    byte * next();
private:
    ExternalRun * externalRun;
    SortedRecordRenderer * inMemoryRenderer;
    TournamentTree * tree;
};