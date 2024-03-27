#include <tuple>
#include <fstream>
#include <memory>
#include "defs.h"

using ofstream_ptr = std::shared_ptr<std::ofstream>;

std::tuple<int, int, string> getArgs(int argc, char* argv[]);
std::tuple<string, string> separatePath(string path);
ofstream_ptr getInFileStream(string outputPath);