# External-Sort

External sort project.

## Getting started

First, intall `argparse` as dependency: [installation](https://github.com/p-ranav/argparse?tab=readme-ov-file#building-installing-and-testing).

Use the command line to run the program:

```bash
Text.exe -c <record count> -s <record size> [-o <output path>] [-t <trace path>] [-i <input path>] [-d <removal-method>]
```

The program accepts the following command line parameters:

- `-c, --count`: the number of records
- `-s, --size`: the size of each record, should be between 20 and 2000
- `-o, --output`: the path of the output result, default to "output.txt"
- `-t, --trace`: the path of the trace log file, default to "trace"
- `-i, --input`: (optional) the path of the input file which stores the record values. If not provided, will generate random records.
- `-d, --duplicate-removal`: (optional) if provided, will remove duplicated records. The `<removal-method>` should be `"instream"` or `"insort"`.

### Existing Test Cases

Below are a few targets in the Makefile:

- `make test`: test the overall function of the program, with random data generation and duplicate removal.
- `make insort`: test the duplicate removal. It generates 4000 records of size 2, so it is guaranteed to have duplicated records. Then, it will use insort method to remove duplicates.
- `make instream`: similar to `make insort`, but use instream method.
- `make 1g`: a test case with 1G data.
- `make 120g`: a huge test case with 120G data (please ensure you have sufficient disk space).

In the Makefile, you can control the level of log output by defining verbosity macros in `CPPFLAGS`:

- (no such macro): prints out key information of the program, such as the number of records generated, whether the final outcome is sorted and whether there are duplicated records.
- `-DVERBOSEL1`: prints out key information of a plan. For example, for `VerifyPlan`, the program will print out the number of rows consumed and produced; for `WitnessPlan`, it will also print out the initial parity.
- `-DVERBOSEL2`: prints out information about each record, such as whether it is a duplicated or an out-of-order record. Warning: will produce huge output.
- `-DPRODUCTION`: (enabled by default) prints out production traces in the form of `[Operation] -> [Type]: <message>`.

## Design Overview

The program consists of a chain of Plan-Iterators that are called FILO, i.e., the last plan is called first and back pressure upstream operators to send data. The chain of plans is as follows, see [Test.cpp](./Test.cpp) for details:

- [ScanPlan](./Scan.h): Reads records from a file or generates random records.
- [witnessPlan](./Witness.h): Processes record stream from `ScanPlan` and calculate and cache a parity value.
- [SortPlan](./Sort.h): Sorts the records, output them to a file, and send them to downstream plans.
- [RemovePlan](./Remove.h): If duplicate removal is enabled with the instream method, this plan removes duplicates in the sorted records.
- [VerifyPlan](./Verify.h): Verifies the sorted records and checks whether there are duplicated records.
- [witnessPlan2](./Witness.h): Processes record stream from `VerifyPlan` and calculate and cache a parity value. The parity values of `witnessPlan` and `witnessPlan2` should be the same, if duplicate removal is not enabled. Otherwise, the chain of plans corrupted the data.

Sorting is divided into a few classes:

- [SortedRecordRenderer](./SortedRecordRenderer.h): A base class that renders records in sorted order, typically using a tournament tree, and produces a run.
- [CacheOptimizedRenderer](./InMemoryRenderer.h): Derived from `SortedRecordRenderer`, it merges in-cache tournament tree and renders records in sorted order in memory. If the data size fits into the memory, a `CacheOptimizedRenderer` will be used to output records one by one in `SortIterator::next()`.
- [`vector<string> SortIterator::_createInitialRuns ()`](./Sort.cpp): If the data size cannot fit into memory, multiple initial in-memory runs (rendered from `CacheOptimizedRenderer`) will be created first and saved as intermediate data.
- [ExternalRenderer](./ExternalRenderer.h): Derived from `SortedRecordRenderer`, it conducts **one pass** of merging runs from files (either in-memory runs or external runs) and renders records in sorted order from the runs.
- [ExternalRun](./ExternalRun.h): Reads an intermediate file and render records in sorted order from that file. One `ExternalRenderer` typically merges multiple `ExternalRun`s.
- [ExternalSorter](./ExternalSorter.h): Takes a list of in-memory runs and merges them into a single `ExternalRenderer`, potentially taking multiple passes. This final renderer will be used to output records one by one in `SortIterator::next()`.

In addition, the in-memory run (while it is being sorted in memory) and the in-memory page of an external run are expressed by a class [Buffer](./Buffer.h). The `Buffer` class manages a contiguous memory block that consist of multiple records. It maintains two pointers: `toBeRead` and `toBeFilled`. A `next()` call is used to scan records from the buffer one by one. For a page of an external run, data is copied from the external run file sequentially page by page. For an in-memory run, alphanumeric data is generated randomly record by record or read from an input file.

## Features Implemented

This section is written assuming that you have read the [Design Overview](#design-overview) section.

### Tournament tree

- [Header file](./TournamentTree.h)
- [Source file](./TournamentTree.cpp)

We implemented a "tree of losers" to merge sorted runs. Key methods:

- `byte * poll ()`: When the buffer that the root comes from is exhausted, advance the farthest loser of the root, push the tree all the way to the top, and poll the root. The latter part of the logic is implemented in `TournamentTree::_advanceToTop()`.
- `byte * pushAndPoll (byte * record)`: When the buffer that the root comes from is not exhausted, use a new record therefrom to push the tree all the way up to poll the root. The latter part of the logic is implemented in `TournamentTree::_advanceToTop()`.
- `Node * _advanceToTop(Node * advancing, Node * incumbent)`: Advance a node, either the farthest loser of the root or a new record from the same buffer as the root, and stand to contest along the way. The loser of each contest stay in that node, and the winner advances to the parent node for the next contest. The process continues until the root is reached. The previous root, guaranteed to be the final winner, is returned. The contest logic is implemented in `tuple<Node *, Node *> _contest(Node * root_left, Node * root_right)`.

### Minimum count of accesses to storage devices

A few techniques are implemented to minimize the number of accesses to storage devices, please refer to each section for details.

- Read ahead buffers
- [Device-optimized page sizes](#device-optimized-page-sizes): Different page sizes are chosen for different storage devices, so that larger batch of data can be read/written together for the slower device.
- Graceful degradation
- [In-sort duplicate removal](#duplicate-removal): This method of duplicate removal eagerly reduces data size, so that less merge efforts are needed afterwards, potentially leading to fewer passes and less spill.

### Duplicate removal

Two methods are implemented to remove duplicated records:

- [In-stream](./Remove.h): After the Sort Plan is executed, duplicate records are placed next to each other in the globally sorted stream of data.
- In-sort: Before the Sort Plan is executed, when a [SortedRecordRenderer](./SortedRecordRenderer.h) at any level is rendering records in sorted order, either locally (e.g. only in an in-memory run of sorted records) or globally, it will remove duplicates. See [`byte * renderRow(std::function<byte *()> retrieveNext, TournamentTree * & tree, ExternalRun * longRun = nullptr)`](./SortedRecordRenderer.cpp).

### Cache-size mini runs

Cache-size mini runs are implemented in class [`CacheOptimizedRenderer`](./InMemoryRenderer.cpp). It first splits the input data into several mini segments so that each segment can be fitted into the cache. Then, for each segment, it creates a cache-size run by building a tournament tree. After building the runs, the renderer builds one tournament tree with leaves pointing to the top of the trees of these cache-size runs.

When doing an in-memory sort, the [`SortIterator::_formInMemoryRenderer`](./sort.cpp) computes the number of rows (records) in cache, and the number of cache-size runs needed (`numCaches`). It creates a tournamant tree for each of the runs. The tree is then passed to `CacheOptimizedRenderer`, where a list of rows of length `numCaches` is formed by polling one element out from each tree, and a master tournament tree is built from these rows. When sorting, this renderer first finds (from the master tree) which tree the smallest element belongs to; then, it tries to get the second largest element from that tree. If such element exists, this element is then pushed into the master tree to maintain the order of the trees. This is repeated until all elements are exhausted.

### Device-optimized page sizes

We choose different page sizes for different storage device. For HDD whose latency is higher, we batch more data to read/write together, so as to minimize the access cost.

A good choice of page size is calculated using the formula `latency * bandwidth`. The page size is defined in `params.h`, respectively `PAGE_SIZE_SSD = 20 KB` and `PAGE_SIZE_HDD = 500 KB`.

Code examples:

- [Materializer](./SortedRecordRenderer.h) is responsible for materializing intermediate and final output. It eagerly tries to write into SSD, whenever there is at least one page size of space available on SSD (implemented in [`Metrics::getAvailableStorage()`](./Metrics.cpp)). Materializer will choose the page size according to what storage device it is writing to. See `Materializer::Materializer(u_int8_t pass, u_int16_t runNumber, SortedRecordRenderer * renderer)`. When SSD is exhausted, it will switch to HDD, and the page size is also chosen according to HDD, see `int Materializer::switchDevice (u_int32_t sizeFilled)`.
- [Materializer](./SortedRecordRenderer.h) also records the device choice of the intermediate output following a file name pattern, e.g. `run0-device0-34460-device1` means that the intermediate output is written to SSD first, after 34460 records, it switches to HDD. See `int Materializer::switchDevice (u_int32_t sizeFilled)`.
- [ExternalRun](./ExternalRun.h) is responsible for reading an intermediate file and render records in sorted order from that file. It first parses the file name and decides which storage device it is reading from. It chooses the page size according to the storage device. It switches device when the run file is stored on both SSD and HDD. See `ExternalRun::ExternalRun (const string &runFileName, RowSize recordSize)`.

### Spilling memory-to-SSD and SSD-to-HDD

Whenever the data size is larger than the memory, in-memory runs are treated as intermediate data and saved to SSD. Two in-memory renderers, `CacheOptimizedRenderer` or `NaiveRenderer`, derived from [SortedRecordRenderer](./SortedRecordRenderer.h), inherits a `materializer`.

Materializer materializes the intermediate data to the fastest available device. That means that the intermediate data is written to SSD when there is one page size of space available. When SSD is exhausted, the intermediate data is written to HDD. One run can span multiple devices, and the device choice is recorded in the file name.

`materialize` flag is turned on by default, because most renderers either produce intermediate data or final output; both cases need to write to storage devices. The `materialize` flag is turned off only in one special case in [graceful degradation](#graceful-degradation).

### Graceful Degradation

#### Into Merging: [`SortedRecordRenderer * SortIterator::gracefulDegradation ()`](./Sort.cpp)

When the data size is only a little larger than the size of a memory run ("a little" defined in `GRACEFUL_DEGRADATION_THRESHOLD` in `params.h`, our default value is 1.5), instead of pivoting into an entire external merge, we degrade gracefully into hybrid merge, only spilling part of the data to SSD, by creating an `initialMemoryRun`. This run will later be read into memory page by page (managed by a [ExternalRun](./ExternalRun.h)). It is merged with the rest of the data in memory (managed by a [CacheOptimizedRenderer](./InMemoryRenderer.h), whose `materialize` is turned off, as we only want to spill the final result from the GracefulRenderer), by a [GracefulRenderer](./GracefulRenderer.h).

- The memory size of the `CacheOptimizedRenderer` is `MEMORY_SIZE - SSD_PAGE_SIZE * 3`, as the `GracefulRenderer` would need one page for output, one page for external renderer run page, one page for external renderer read-ahead.
- Therefore, the `initialMemoryRun` needs to be of size `TOTAL_DATA_SIZE - MEMORY_SIZE + SSD_PAGE_SIZE * 3`.

#### Beyond one merge step: [`SortedRecordRenderer * ExternalSorter::gracefulMerge (vector<string>& runNames, int basePass, int rendererNum)`](./ExternalSorter.cpp)

When the runs can barely fit into one pass (`allMemoryNeeded <= MEMORY_SIZE * GRACEFUL_DEGRADATION_THRESHOLD`), instead of merging them in two pass, we gracefully degrade the merging. We merge some of the runs in an `ExternalRenderer` and spill the result to storage devices by calling `string SortedRecordRenderer::run()` which returns the file name. Then, we merge the rest of the runs with the spilled run in another `ExternalRenderer`.

The gracefulMerge algorithm is expressed as an optimization problem:

- Condition: The final `ExternalRenderer` should be able to fit into memory.
- Optimization: Minimize the size of the spilled run.

Refer to the file for the actual code.

Once we find the smallest number of runs to merge into an `initialRun`, we call `ExternalRenderer::run()` to spill the result to storage devices. We then push the spilled run to the rest of runs and merge them in another `ExternalRenderer`.

### Optimized merge patterns: 

Relevant functions:

- [`ExternalRenderer::ExternalRenderer (RowSize *recordSize*, vector<string> *runFileNames*, u_int64_t *readAheadSize*,u_int8_t *pass*, u_int16_t *rendererNumber*, bool *removeDuplicates*)`](./ExternalRenderer.cpp)
-  [`byte * SortedRecordRenderer::renderRow(std::function<byte *()> *retrieveNext*, TournamentTree *& *tree*, ExternalRun * *longRun*) `](./SortedRecordRenderer.cpp)

We implemented one technique to optimize the merge pattern to bound the bad performance of a special case in external merge that one run is extremely long. We define extremely long as when that one is `LONG_RUN_THRESHOLD`x of the total size of other runs, the default value being 2. 

Instead of having every record of this long run go through a series of contests to merge from the tournament tree, we only build a tree with other runs, compare the root with the next record from the long run, and render the smaller of the two. You can think of this technique as *extending the tree to have one more level at the top*---have one more contest between the long run and the smallest from the rest of the runs. You can also think of it as search a small number of records in sorted order in a sorted long run.

### Verifiers

- Two [Witness](./Witness.h)es: The witness does an integrity check on the data to ensure no record is altered during the sort by calculating a parity of all records before and after sort. Note that the parity will not match with duplicate removal. The witness creates a check record of the same size as the input records, with initial value `0xFF` for all bytes. It then loops through all the given data, and apply an XOR to the check record with the given record (i.e., `checkRecord = checkRecord xor inputRecord`). The final value of the check record will be displayed at the end.
- [Verify](./Verify.h): The VerifyPlan is another check and inspect the following 3 properties of the data after sort:
  - Whether records are sorted.
  - Whether there are duplicates.
  - Whether there are non-alphanumeric characters in the records.
  
  The VerifyPlan reads in the outcome of the SortPlan and maintains the "previous record". If current record is larger than the previous record, then the outcome is not sorted ascending; if current record is equal to the previous record, then there are duplicates in the outcome.

### Bonus implementation: Read ahead buffers

Relevant functions:

- [`void ExternalRun::readAhead()`](./ExternalRun.cpp)
- [`ExternalRenderer::ExternalRenderer (RowSize recordSize, vector<string>::const_iterator runFileNames_begin, vector<string>::const_iterator runFileNames_end, u_int64_t readAheadSize, u_int8_t pass, u_int16_t rendererNumber, bool removeDuplicates)`](./ExternalRenderer.cpp)
- [`tuple<u_int64_t, u_int64_t> ExternalSorter::profileReadAheadAndOutput (const vector<string>& runNames, u_int16_t mergedRunCount)`](./ExternalSorter.cpp)

As an additional optimization technique, we implemented read ahead buffers for an `ExternalRenderer`. The read ahead buffer is used to read data from the run files in advance, whose pages are exhausted faster than other runs.

All `ExternalRun`s in an `ExternalRenderer` share `READ_AHEAD_SIZE`. "Exhausted faster" is defined by which run reaches a threshold value `READ_AHEAD_THRESHOLD` first. Both `READ_AHEAD_SIZE` and `READ_AHEAD_THRESHOLD` are implemented as static variables in `ExternalRun` and are initialized in the constructor of `ExternalRenderer`.

Typically, extra memory are allocated for at least 2 such buffers. The size of each buffer depends on the distribution of the run files on storage devices. Currently, when 67% or more of the runs are on HDD, 2 are both of HDD page size; when 33%--67% of the runs are on HDD, 1 is of SSD page size and 1 is of HDD page size; otherwise, 2 are both of SSD page size. The actual profiling algorithm is implemented in `ExternalSorter::profileReadAheadAndOutput`. After allocating memory to all input pages, output pages, and the profiled read ahead size, extra memory is additionally allocated for the read ahead buffers.

## Appendix: Documentation

### Metrics

We defined a class `Metrics` to record all access to SSD and HDD that can be accessed everywhere using global static functions. To use:

- At program start, call `Metrics::Init()` to initialize.
- Two macros are defined: `STORAGE_SSD` and `STORAGE_HDD`, representing which storage you are accessing.
- To get the stats, call `Metrics::getMetrics(device_type)`. It returns a struct type `StorageMetrics` with 7 attributes:
  - `double dataTransferCost`: the total time of data transfer of this storage device. Note that `(numBytesRead + numBytesWritten) / bandwidth` is not necessarily equal to `dataTransferCost`, because read ahead does not block IO and is not counted in `dataTransferCost`.
  - `double accessCost`: the total access cost of this storage device.
  - `u_int64_t numBytesRead`: number of bytes read from this storage device, including read ahead
  - `u_int64_t numBytesWritten`: number of bytes written to this storage device
  - `u_int64_t numAccessesRead`: number of read accesses to this storage device
  - `u_int64_t numAccessesWritten`: number of write accesses to this storage device
- The specific parameters are defined in `params.h`.
