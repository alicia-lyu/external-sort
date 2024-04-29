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
- TODO@Yuheng

In the Makefile, you can control the level of log output by defining verbosity macros in `CPPFLAGS`:

- (no such macro): prints out key information of the program, such as the number of records generated, whether the final outcome is sorted and whether there are duplicated records.
- `-DVERBOSEL1`: (default) prints out key information of a plan. For example, for `VerifyPlan`, the program will print out the number of rows consumed and produced; for `WitnessPlan`, it will also print out the initial parity.
- `-DVERBOSEL2`: prints out information about each record, such as whether it is a duplicated or an out-of-order record. Warning: will produce huge output.

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

TODO@Yuheng

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

- Two [Witness](./Witness.h)es: TODO@Yuheng
- [Verify](./Verify.h): TODO@Yuheng

### Bonus: Read ahead buffers

TODO@Alicia

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
