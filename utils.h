#pragma once

#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"
#include "Buffer.h"
#include <argparse/argparse.hpp>

using std::string;
using std::tuple;
using argparse::ArgumentParser;

struct Config {
    RowCount recordCount;
    RowSize recordSize;
    string outputPath;
    string inputPath;
    bool removeDuplicates;
};

Config getArgs(int argc, char* argv[]);
tuple<string, string> separatePath(string path);
string byteToHexString(byte byte);
string rowToHexString(byte * rowContent, RowSize size);
u_int32_t getRecordCountPerRun(RowSize recordSize, bool inSSD);
string rowToString(byte * rowContent, RowSize size);
string rowRawValueToString(byte * rowContent, RowSize size);