#pragma once

#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"
#include "Buffer.h"
#include <argparse/argparse.hpp>
#include "TournamentTree.h"

using std::string;
using std::tuple;
using std::vector;
using argparse::ArgumentParser;

struct Config {
    RowCount recordCount;
    RowSize recordSize;
    string tracePath;
    string outputPath;
    string inputPath;
    bool removeDuplicates;
    string removalMethod;
};

Config getArgs(int argc, char* argv[]);
tuple<string, string> separatePath(string path);
string byteToHexString(byte byte);
string rowToHexString(byte * rowContent, RowSize size);
u_int32_t getRecordCountPerRun(RowSize recordSize, bool inSSD); // TODO: use Metrics::getAvailableStorage
string rowToString(byte * rowContent, RowSize size);
string rowRawValueToString(byte * rowContent, RowSize size);
tuple<vector<u_int8_t>, vector<u_int64_t>> parseDeviceType(const string &filename);
u_int8_t getLargestDeviceType(const string &filename);