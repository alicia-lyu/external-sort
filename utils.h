#pragma once

#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"
#include "Data.h"

std::tuple<int, int, string> getArgs(int argc, char* argv[]);
std::tuple<string, string> separatePath(string path);
std::string byteToHexString(byte byte);
string rowToHexString(byte * rowContent, RowSize size);