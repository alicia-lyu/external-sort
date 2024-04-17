#pragma once

#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"
#include "Buffer.h"

std::tuple<int, int, string> getArgs(int argc, char* argv[]);
std::tuple<string, string> separatePath(string path);
std::string byteToHexString(byte byte);
string rowToHexString(byte * rowContent, RowSize size);
u_int32_t getRecordCountPerRun(RowSize recordSize, bool inSSD);
string rowToString(byte * rowContent, RowSize size);
string rowRawValueToString(byte * rowContent, RowSize size);