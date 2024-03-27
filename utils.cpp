#include <filesystem>
#include <fstream>
#include <iostream>
#include "Iterator.h"

std::tuple<int, int, std::string> getArgs(int argc, char* argv[])
{
    RowCount recordCount;
    u_int16_t recordSize; // 20-2000 bytes
    std::string outputPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-c" && i + 1 < argc) {
            recordCount = (RowCount) argv[++i];
        } else if (arg == "-s" && i + 1 < argc) {
            recordSize = (u_int16_t) argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        }
    }

    return std::make_tuple(
        recordCount, recordSize, outputPath
    );
}

char separator = std::filesystem::path::preferred_separator;

std::tuple<std::string, std::string> separatePath(std::string path)
{
    size_t lastSlash = path.rfind(separator);

    if (lastSlash == std::string::npos) {
        return std::make_tuple("", path);
    }
    
    return std::make_tuple(
        path.substr(0, lastSlash), path.substr(lastSlash+1));
}

std::ifstream getInFileStream(std::string outputPath)
{
    std::string dir, filename;
    tie(dir, filename) = separatePath(outputPath);
    std::string inputPath = dir + separator + "input.txt";

    std::ifstream inFile(inputPath);

    if (!inFile.is_open()) {
        std::cerr << "Failed to open input file: " << inputPath << std::endl;
        exit(1);
    }

    return inFile;
}