#pragma once

#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"
#include "Buffer.h"
#include <argparse/argparse.hpp>
#include <functional>
#include "TournamentTree.h"

using std::string;
using std::tuple;
using std::vector;
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
byte * renderRow(std::function<byte *()> retrieveNext, 
    std::function<byte *(byte * rendered)> copyRowToOutputBuffer,
    TournamentTree * renderingTree,
    byte * lastRow,
    bool removeDuplicates, 
    RowSize recordSize);
tuple<vector<u_int8_t>, vector<u_int64_t>> parseDeviceType(string filename);
u_int8_t getLargestDeviceType(string filename);