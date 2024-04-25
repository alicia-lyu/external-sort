#include <vector>
#include <fstream>

#include "Iterator.h"
#include "utils.h"
#include "defs.h"
#include "Buffer.h"

class ScanPlan : public Plan
{
	friend class ScanIterator;
public:
	ScanPlan (RowCount const count, RowSize const size, const string &inputPath); 
	~ScanPlan ();
	Iterator * init () const;
private:
	RowCount const _count;
	RowSize const _size;
	u_int32_t const _countPerRun;
	const string & _inputPath;
}; // class ScanPlan

class ScanIterator : public Iterator
{
public:
	ScanIterator (ScanPlan const * const plan);
	~ScanIterator ();
	byte * next ();
private:
	ScanPlan const * const _plan;
	std::ofstream _inputFile;
	u_int16_t _scanCount; // max. 2^16 = 65536, total 120 GB, each scan 1.8 MB
	u_int32_t _countPerScan; // max. 1.8 MB / 20 B = 2^17
	RowCount _count;
	Buffer * _run;
	string _getInputFileName();
}; // class ScanIterator