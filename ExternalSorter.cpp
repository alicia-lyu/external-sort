// skeleton of all methods:

#include "ExternalSorter.h"
#include "utils.h"

ExternalSorter::ExternalSorter (vector<string>& runNames, RowSize recordSize, bool removeDuplicates) 
: recordSize(recordSize), removeDuplicates(removeDuplicates), runNames(runNames)
{
    TRACE (false);
}

ExternalSorter::~ExternalSorter () 
{
    TRACE (false);
}

SortedRecordRenderer * ExternalSorter::init () {
	u_int8_t pass = 0;
	SortedRecordRenderer * renderer = nullptr;
	// Multi-pass merge
	while (runNames.size() > 1) { // Need another pass to merge the runs
		++ pass;

		u_int16_t rendererNum = 0;

		// Gracefully merge all in one pass if possible (this is the last pass if there is only one renderer)
		u_int64_t allMemoryNeeded = calculateMemoryForAll(runNames);
		if (allMemoryNeeded <= MEMORY_SIZE * GRACEFUL_DEGRADATION_THRESHOLD && allMemoryNeeded > MEMORY_SIZE) {
			std::vector<string> restOfRuns = std::vector<string>(runNames.begin(), runNames.end());
			renderer = gracefulMerge(restOfRuns, pass, rendererNum);
			return renderer;
		}

		u_int16_t mergedRunCount = 0;
		vector<string> mergedRunNames;

		while (mergedRunCount < runNames.size()) { // has more runs to merge, may still be the last pass if  allMemoryNeeded <= MEMORY_SIZE
			u_int16_t mergedRunCountSoFar = mergedRunCount;
			auto [mergedRunCountNew, readAheadSize] = assignRuns(runNames, mergedRunCountSoFar);
			mergedRunCount = mergedRunCountNew;
			#if defined(VERBOSEL1) || defined(VERBOSEL2)
			traceprintf ("Pass %d renderer %d: Merging runs from %d to %d out of %zu\n", pass, rendererNum, mergedRunCountSoFar, mergedRunCount - 1, runNames.size() - 1);
			#endif
			renderer = new ExternalRenderer(recordSize, 
			vector<string>(runNames.begin() + mergedRunCountSoFar, runNames.begin() + mergedRunCount), 
			readAheadSize, pass, rendererNum, removeDuplicates);

			if (rendererNum == 0 && mergedRunCount == runNames.size()) // The last renderer in the last pass
			{
				return renderer;
			} else { // Need another pass for merged runs
				mergedRunNames.push_back(renderer->run());
				rendererNum++;
				delete renderer;
			}
		}

		runNames = mergedRunNames;
	}

	throw std::runtime_error("External sort not returning renderer.");
}

tuple<u_int16_t, u_int64_t> ExternalSorter::assignRuns(vector<string>& runNames, u_int16_t mergedRunCount) 
{
	auto [readAheadSize, outputPageSize] = profileReadAheadAndOutput(runNames, mergedRunCount);

	// INPUT BUFFERS
	u_int64_t memoryConsumption = readAheadSize + outputPageSize;
	while (mergedRunCount < runNames.size()) { // max. 120 G / 98 M = 2^11

		string &runName = runNames.at(mergedRunCount);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;

		if (memoryConsumption + pageSize > MEMORY_SIZE) break;

		memoryConsumption += pageSize;
		mergedRunCount++;
	}

	Assert (memoryConsumption <= MEMORY_SIZE, __FILE__, __LINE__);
	readAheadSize += MEMORY_SIZE - memoryConsumption; // Use the remaining memory for more read-ahead buffers if any

	return std::make_tuple(mergedRunCount, readAheadSize);
}

u_int64_t ExternalSorter::calculateMemoryForAll (vector<string>& runNames)
{
	auto [readAheadSize, outputPageSize] = profileReadAheadAndOutput(runNames, 0);

	// Calculate all memory consumption till the end of runNames
	u_int64_t allMemoryConsumption = readAheadSize + outputPageSize;
	for (int i = 0; i < runNames.size(); i++) {
		auto deviceType = getLargestDeviceType(runNames.at(i));
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		allMemoryConsumption += pageSize;
	}

	return allMemoryConsumption;
}

tuple<u_int64_t, u_int64_t> ExternalSorter::profileReadAheadAndOutput (vector<string>& runNames, u_int16_t mergedRunCount)
{
	TRACE (false);
	// A conservative estimate of the output page size
	// The profiled outputFileSize is the largest possible
	u_int64_t memoryConsumption = 0;
	u_int64_t outputFileSize = 0;
	u_int16_t ssdRunCount = 0;
	u_int16_t hddRunCount = 0;
	while (mergedRunCount < runNames.size()) {

		string &runName = runNames.at(mergedRunCount);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;

		if (memoryConsumption + pageSize > MEMORY_SIZE) break;

		memoryConsumption += pageSize;
		mergedRunCount++;
		outputFileSize += std::filesystem::file_size(runName);
		if (deviceType == 0) ssdRunCount++;
		else hddRunCount++;
	}

	// Output page size
	int deviceType = Metrics::getAvailableStorage(outputFileSize);
	// Allocate output buffer conservatively: max page sizes of all storage devices needed
	// But will try to write into SSD first if possible
	auto outputPageSize = Metrics::getParams(deviceType).pageSize;

	// Read-ahead buffer size
	double hddRunRatio = (double) hddRunCount / (hddRunCount + ssdRunCount);
	int hddBufferCount;
	if (hddRunRatio > 0.67) hddBufferCount = 2;
	else if (hddRunCount > 0.33) hddBufferCount = 1;
	else hddBufferCount = 0;
	u_int64_t readAheadSize = hddBufferCount * HDD_PAGE_SIZE + 
		(READ_AHEAD_BUFFERS_MIN - hddBufferCount) * SSD_PAGE_SIZE;
	return std::make_tuple(readAheadSize, outputPageSize);
}

SortedRecordRenderer * ExternalSorter::gracefulMerge (vector<string>& runNames, int basePass, int rendererNum)
{
	TRACE (false);

	// Optimization problem:
	// Memory consumption of the graceful renderer <= MEMORY_SIZE
	// Minimize the size of the initial run so that 

	auto [gracefulReadAheadSize, ret2] = profileReadAheadAndOutput(runNames, 0);
	// Only the first few runs whose pages can fit into memory are profiled --- for graceful renderer
	// It is assumed that the initial renderer would have plenty of available memory for reading ahead
	// Because we are minimizing the size of the initial run

	int n = runNames.size();

	u_int64_t inputMemoryForAllRuns = 0; // Input buffer size to contain pages from all runs
	u_int64_t allRunSize = 0; // Total size of all runs
	for (int i = 0; i < n; i++) {
		auto deviceType = getLargestDeviceType(runNames.at(i));
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		inputMemoryForAllRuns += pageSize;
		allRunSize += std::filesystem::file_size(runNames.at(i));
	}

	int const gracefulOutputDevice = Metrics::getAvailableStorage(allRunSize);
	
	u_int64_t initialRunSize = 0;
	u_int64_t initialPageSize;
	u_int64_t initialInputMemory = 0; // output page size for initialRenderer, input page size of initialRun for gracefulRenderer
	u_int64_t const gracefulOutputPage = Metrics::getParams(gracefulOutputDevice).pageSize;
	u_int64_t const gracefulMemoryFixed = gracefulOutputPage + gracefulReadAheadSize;

	// Find the smallest initial run size
	int i;
	for (i = 1; i <= n; i++) {
		// the last i runs are merged in the initial renderer
		string runName = runNames.at(n - i);
		initialRunSize += std::filesystem::file_size(runName);
		auto deviceType = getLargestDeviceType(runName);
		auto pageSize = Metrics::getParams(deviceType).pageSize;
		initialInputMemory += pageSize;

		auto initialRunDevice = Metrics::getAvailableStorage(initialRunSize);
		initialPageSize = Metrics::getParams(initialRunDevice).pageSize; 
		if (gracefulMemoryFixed + inputMemoryForAllRuns - initialInputMemory + initialPageSize <= MEMORY_SIZE) // Memory consumption of the graceful renderer
		{
			break;
		}
	}

	u_int64_t initialReadAheadSize = MEMORY_SIZE - initialInputMemory - initialPageSize;
	Assert (initialReadAheadSize >= 0, __FILE__, __LINE__);

	ExternalRenderer * initialRenderer = new ExternalRenderer(recordSize, 
		vector<string>(runNames.end() - i, runNames.end()),
		initialReadAheadSize, basePass, rendererNum + 1, removeDuplicates
	); // rendererNum + 1: This run will not be pushed back to mergedRunNames in _externalSort
	string initialRunFileName = initialRenderer->run();

	// Create the graceful renderer for the rest of the runs + initial run
	vector<string> restOfRuns = std::vector<string>(runNames.begin(), runNames.end() - i);
	restOfRuns.push_back(initialRunFileName);
	SortedRecordRenderer * gracefulRenderer = new ExternalRenderer(recordSize, restOfRuns, gracefulReadAheadSize, basePass, rendererNum, removeDuplicates);

	return gracefulRenderer;
	
}

