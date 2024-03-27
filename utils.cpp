#include <filesystem>
#include <iostream>
#include <cstdlib>
#include "Iterator.h"
#include "utils.h"

std::tuple<int, int, string> getArgs(int argc, char* argv[])
{
    RowCount recordCount;
    RowSize recordSize; // 20-2000 bytes
    string outputPath;

    if (argc <= 3) {
        std::cerr << "Usage: " << argv[0] << " -c <record_count> -s <record_size> -o <output_path>\n";
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-c" && i + 1 < argc) {
            recordCount = (RowCount) argv[++i];
        } else if (arg == "-s" && i + 1 < argc) {
            char* end;
            unsigned long value = std::strtoul(argv[++i], &end, 10); // Convert string to unsigned long

            if (*end != '\0' || value < 20 || value > 2000) {
                std::cerr << "Expected record size between 20 and 2000, got " << value << '\n';
                exit(1);
            }

            RowSize recordSize = static_cast<RowSize>(value);
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        }
    }

    return std::make_tuple(
        recordCount, recordSize, outputPath
    );
}

char separator = std::filesystem::path::preferred_separator;

std::tuple<string, string> separatePath(string path)
{
    size_t lastSlash = path.rfind(separator);

    if (lastSlash == string::npos) {
        return std::make_tuple("", path);
    }
    
    return std::make_tuple(
        path.substr(0, lastSlash), path.substr(lastSlash+1));
}

ofstream_ptr getInFileStream(string outputPath)
{
    string dir, filename;
    std::tie(dir, filename) = separatePath(outputPath);
    string inputPath = dir + separator + "input.txt";

    auto inFile = std::make_shared<std::ofstream>(inputPath);

    if (!inFile->is_open()) {
        std::cerr << "Failed to open input file: " << inputPath << std::endl;
        exit(1);
    }

    return inFile;
}