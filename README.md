# External-Sort

External sort project.

## Getting started

First, intall `argparse` as dependency: [installation](https://github.com/p-ranav/argparse?tab=readme-ov-file#building-installing-and-testing).

Use the command line to run the program:

```bash
Text.exe -c <record count> -s <record size> -o <output path> [-i <input path>] [-d]
```

The program accepts the following command line parameters:

- `-c, --count`: the number of records
- `-s, --size`: the size of each record, should be between 20 and 2000
- `-o, --output`: the path of the output log
- `-i, --input`: (optional) the path of the input file which stores the record values. If not provided, will generate random records.
- `-d, --duplicate-removal`: (optional) if provided, will remove duplicated records after sort.

## Testing

Below are a few targets in the Makefile:

- `make test`: test the overall function of the program, with random data generation and duplicate removal.
- `make dup`: test the duplicate removal. It generates 4000 records of size 2, so it is guaranteed to have duplicated records.
- (To be finished)

In the Makefile, you can control the level of log output by defining verbosity macros in `CPPFLAGS`:

- (no such macro): prints out key information of the program, such as the number of records generated, whether the final outcome is sorted and whether there are duplicated records.
- `-DVERBOSEL1`: (default) prints out key information of a plan. For example, for `VerifyPlan`, the program will print out the number of rows consumed and produced; for `WitnessPlan`, it will also print out the initial parity.
- `-DVERBOSEL2`: prints out information about each record, such as whether it is a duplicated or an out-of-order record. Warning: will produce huge output.

