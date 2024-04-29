#include <vector>
#include "ExternalRenderer.h"

using std::vector;
using std::string;

// Multi-pass external sort
class ExternalSorter {
public:
    ExternalSorter (vector<string>& runNames, RowSize recordSize, bool removeDuplicates = false);
    ~ExternalSorter ();
    SortedRecordRenderer * init (); // Return the renderer of the last pass that is ready to render globally sorted records
private:
    RowSize recordSize;
    bool removeDuplicates;
    vector<string> runNames;
    tuple<u_int64_t, u_int64_t> profileReadAheadAndOutput(vector<string>& runNames, u_int16_t mergedRunCount); 
	tuple<u_int16_t, u_int64_t> assignRuns(vector<string>& runNames, u_int16_t mergedRunCount); // The first return is updated merged run count [initial merged run count, returned merged run count) is the assigned runs; the second is the read ahead size
	u_int64_t allMemoryForRestRuns (vector<string>& runNames, u_int16_t mergedRunCount);
	SortedRecordRenderer * gracefulMerge (vector<string>& runNames, int basePass, int rendererNum);
};