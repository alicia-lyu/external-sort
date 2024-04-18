#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include "Iterator.h"
#include "utils.h"

/*
Return value:
    * recordCount: Number of records to generate
    * recordSize: Size of each record, must be 20-2000
    * outputPath: Output path
    * inputPath: Input path. If not provided, value is empty
    * removeDuplicates: remove duplicates, default is false
*/
Config getArgs(int argc, char* argv[])
{
    ArgumentParser parser("sort");

    /*
    Arguments:
        * -c, --count: Number of records to generate
        * -s, --size: Size of each record, must be 20-2000
        * -o, --output: Output path
        * (optional) -i, --input: Input path. If not provided, generate records
        * (optional) -d, --duplicate-removal: remove duplicates
    */

    parser.add_argument("-c", "--count")
        .scan<'d', RowCount>()
        .help("Number of records to generate")
        .required();

    parser.add_argument("-s", "--size")
        .scan<'d', RowSize>()
        .help("Size of each record, must be 20-2000")
        .required();

    parser.add_argument("-o", "--output")
        .help("Output path")
        .required();

    parser.add_argument("-i", "--input")
        .help("Input path")
        .default_value(string(""));
    
    parser.add_argument("-d", "--duplicate-removal")
        .help("Remove duplicates")
        .flag();

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cout << err.what() << std::endl;
        std::cout << parser;
        exit(1);
    }

    Config config;
    config.recordCount = parser.get<RowCount>("-c");
    config.recordSize = parser.get<RowSize>("-s");
    config.outputPath = parser.get<string>("-o");
    config.inputPath = parser.get<string>("-i");
    config.removeDuplicates = parser.get<bool>("-d");

    // check record size range
    if (config.recordSize < 20 || config.recordSize > 2000) {
        std::cerr << "Expected record size between 20 and 2000, got " << config.recordSize << '\n';
        exit(1);
    }

    return config;
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

u_int32_t getRecordCountPerRun(RowSize recordSize, bool inSSD) {
    u_int32_t recordCountPerRun = inSSD ? (MEMORY_SIZE - SSD_PAGE_SIZE) / recordSize : (MEMORY_SIZE - HDD_PAGE_SIZE) / recordSize;
    return recordCountPerRun;
}

// convert a row of bytes to a string
// the bytes are already converted to alphanumeric characters
string rowToString(byte * rowContent, RowSize size) {
    string result(size, ' ');
    for (int i = 0; i < size; ++i) {
        byte byte = rowContent[i];
        result[i] = byte;
    }
    return result;
}

// convert a row of bytes to a string
// where the bytes are represented as their raw values
string rowRawValueToString(byte * rowContent, RowSize size) {
    string result;
    for (int i = 0; i < size; ++i) {
        byte byte = rowContent[i];
        result += std::to_string(byte) + " ";
    }
    return result;
}