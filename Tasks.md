# Tasks

Rough Timeline   | Tasks
----------------|----------------------------------------
February 12, 2024 | Pair Finalisation, Complete Code walk through
February 26, 2024 | Define class for data records
March 11, 2024    | Add predicate evaluation to FilterIterator
March 25, 2024    | Add in-memory sorting, duplicate removal
April 8, 2024     | Add Plan & Iterator that verify a set of rows, Performance Testing and optimization
April 22, 2024    | Performance Testing and optimization
April 29, 2024    | Submission


## Milestones

### Infrastructure: By March 11

- [x] Trace existing code and disable (not remove!) excessive tracing output @Alicia
- [x] Define class for data records @Alicia @Yuheng
- [x] Add data generation (random values) in ScanIterator @Alicia @Yuheng
- [x] Test with simple plan -- scan only @Alicia
- [x] Add parity check with new classes Witness @Alicia @Yuheng
- [x] Add Plan & Iterator that verify the order of the sorted rows @Yuheng

### Sorting: By April 8

- [x] Write and test the tournament tree @Alicia
- [x] Add in-memory sorting, test with 0, 1, 2, 3, 7 rows @Alicia
- [x] Add multi-level external sort that spills to SSD, test with 0, 1, 2, 3, 10, 29, 100, 576, 1000 rows @Alicia
- [x] Add external sort that spills to HDD @Alicia @Yuheng
- [x] Add HDD and SSD metrics: @Yuheng @Alicia
  - SSD: 0.1 ms latency, 200 MB/s bandwidth
  - HDD: 5 ms latency, 100 MB/s bandwidth
- [ ] Test with 10^3 * 50 (50M), 10^3 * 125 (125M), 10^5 * 120 (12 G), 10^6 * 120 (120 G) (rows, record size) @Yuheng
- [x] Test with sample input provided by TA @Yuheng

### Optimization and bonus points: By April 29

- [x] Add in-cache sorting and test again: In addition to in-memory sorting @Yuheng
- [x] Add duplicate removal and evaluate performance (distinct) @Yuheng
  - [x] In stream (after sort) @Yuheng
  - [x] In sort @Yuheng
- [x] Add graceful degradation
  - [x] Into merging @Alicia
  - [ ] Beyond one merge step @Yuheng
- [x] Add 2 read-ahead buffers @Alicia
- [x] Add optimized merge pattern @Alicia