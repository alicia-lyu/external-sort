#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <sstream>
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
            recordCount = (RowCount) std::stoi(argv[++i]);
        } else if (arg == "-s" && i + 1 < argc) {
            char* end;
            unsigned long value = std::strtoul(argv[++i], &end, 10); // Convert string to unsigned long

            if (*end != '\0' || value < 20 || value > 2000) {
                std::cerr << "Expected record size between 20 and 2000, got " << value << '\n';
                exit(1);
            }

            recordSize = static_cast<RowSize>(value);
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        }
    }

    return std::make_tuple(
        recordCount, recordSize, outputPath
    );
}

std::tuple<string, string> separatePath(string path)
{
    size_t lastSlash = path.rfind(SEPARATOR);

    if (lastSlash == string::npos) {
        return std::make_tuple("", path);
    }
    
    return std::make_tuple(
        path.substr(0, lastSlash), path.substr(lastSlash+1));
}

string byteToHexString(byte byte) {
    std::stringstream result;
    result << "0x"
       << std::hex // Use hexadecimal format
       << std::uppercase // Optional: Use uppercase letters for A-F
       << std::setw(2) // Ensure the output is at least two digits
       << std::setfill('0') // Fill with leading zeros if necessary
       << static_cast<int>(byte); // Convert byte to int for correct formatting
    return result.str();
}

string rowToHexString(byte * rowContent, RowSize size) {
    size = std::max(size, (RowSize) 20); // Curtail long output/logs
    string result = "";
    for (int i = 0; i < size; ++i) {
		byte byte = rowContent[i];
		string hexString = byteToHexString(byte);
        result += hexString + " ";
	}
    return result;
}