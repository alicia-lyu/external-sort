#include "TournamentTree.h"
#include <vector>

class SortedRecordRenderer
{
public:
	SortedRecordRenderer (TournamentTree * tree, std::vector<TournamentTree *> cacheTrees, std::vector<string> runFileNames);
	~SortedRecordRenderer ();
	byte * next ();
	void print();
private:
	TournamentTree * _tree;
	std::vector<TournamentTree *> _cacheTrees;
	std::vector<string> _runFileNames;
};