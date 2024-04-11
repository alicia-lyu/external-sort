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

- [x] Trace existing code @Alicia
- [x] Disable (not remove!) excessive tracing output @Alicia
- [x] Define class for data records @Alicia
- [x] Add data records (incl mem mgmt) to iterators @Alicia
- [x] Add data generation (random values) in ScanIterator @Alicia
- [x] Test with simple plan -- scan only @Alicia
- [x] Add parity check with new classes Witness @Alicia @Yuheng

### Sorting: By April 8

- [x] Write and test the tournament tree @Alicia
- [x] Add in-memory sorting @Alicia
- [ ] Add Plan & Iterator that verify the order of the sorted rows
- [ ] Test with 0, 1, 2, 3, 7 rows
- [ ] Add in-cache sorting and test again: In addition to in-memory sorting, just make sure memory jump doesn't exceed cache line in one round of sorting
- [ ] Add external sort that spills to SSD (including metrics)
- [ ] Test with 0, 1, 2, 3, 10, 29, 100, 576, 1000 rows
- [ ] Add external sort that spills to HDD (including metrics)
- [ ] Test with 10^3 * 50 (50M), 10^3 * 125 (125M), 10^5 * 120 (12 G), 10^6 * 120 (120 G) (rows, record size)

### Optimization and bonus points: By April 29

- [ ] Add predicate evaluation (e.g. parity) to FilterIterator
- [ ] Add duplicate removal and evaluate performance (distinct)
  - [ ] In stream (after sort)
  - [ ] In sort
- [ ] Add graceful degradation