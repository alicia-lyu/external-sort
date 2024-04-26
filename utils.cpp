#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <regex>
#include "Iterator.h"
#include "utils.h"

/*
Return value:
    * recordCount: Number of records to generate
    * recordSize: Size of each record, must be 20-2000
    * tracePath: trace log path
    * outputPath: Output path
    * inputPath: Input path. If not provided, value is empty
    * removeDuplicates: remove duplicates, default is false
    *    if provided, also provides <removal-method> 
*/
Config getArgs(int argc, char* argv[])
{
    ArgumentParser parser("sort");

    /*
    Arguments:
        * -c, --count: Number of records to generate
        * -s, --size: Size of each record, must be 20-2000
        * -t, --trace: Trace log path. Default is "trace"
        * (optional) -o, --ouput: Output path. If not provided, default to "output_table"
        * (optional) -i, --input: Input path. If not provided, generate records
        * (optional) -d, --duplicate-removal: Remove duplicates. Default is false. If provided, also provides <removal-method>
    */

    parser.add_argument("-c", "--count")
        .scan<'d', RowCount>()
        .help("Number of records to generate")
        .required();

    parser.add_argument("-s", "--size")
        .scan<'d', RowSize>()
        .help("Size of each record, must be 20-2000")
        .required();

    parser.add_argument("-t", "--trace")
        .help("Trace log path. If not provided, default to 'trace'")
        .default_value(string("trace"));

    parser.add_argument("-o", "--output")
        .help("Output path. If not provided, default to 'output_table'")
        .default_value(string("output_table"));

    parser.add_argument("-i", "--input")
        .help("Input path. If not provided, generate random records")
        .default_value(string(""));
    
    parser.add_argument("-d", "--duplicate-removal")
        .help("Remove duplicates by providing the removal method, either 'insort' or 'instream'");

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
    config.tracePath = parser.get<string>("-t");
    config.outputPath = parser.get<string>("-o");
    config.inputPath = parser.get<string>("-i");

    // first check if -d is provided
    if (parser.present("-d")) {
        config.removeDuplicates = true;
        config.removalMethod = parser.get<string>("-d");

        // if it's not insort or instream, raise an error
        if (config.removalMethod != "insort" && config.removalMethod != "instream") {
            std::cerr << "Error: Expected removal method to be insort or instream, got " << config.removalMethod << '\n';
            exit(1);
        }
    } else {
        config.removeDuplicates = false;
    }

    // check record size range
    if (config.recordSize < 20 || config.recordSize > 2000) {
        std::cerr << "Warning: Expected record size between 20 and 2000, got " << config.recordSize << '\n';
        // do not exit, let the program continue
        // exit(1);
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
    size = std::min(size, (RowSize) 20); // Curtail long output/logs
    string result = "";
    for (int i = 0; i < size; ++i) {
		byte byte = rowContent[i];
		string hexString = byteToHexString(byte);
        result += hexString + " ";
	}
    // get rid of the last space
    result.pop_back();
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

byte * renderRow(std::function<byte *()> retrieveNext, std::function<byte *(byte * rendered)> copyRowToOutputBuffer,
    TournamentTree * renderingTree, byte * lastRow, bool removeDuplicates, RowSize recordSize) 
{
    byte * rendered, * retrieved;
    byte * output = nullptr;

	while (true) {
        rendered = renderingTree->peekRoot();
        // if no more rows, jump out
		if (rendered == nullptr) break;
        
		if (
            !removeDuplicates ||  // not removing duplicates
            lastRow == nullptr || // last row is null
            memcmp(lastRow, rendered, recordSize) != 0 // last row is different from the current row
        ) {
            output = copyRowToOutputBuffer(rendered); 
            // copy before retrieving next, as retrieving next could overwrite the current page in ExternalRenderer
            retrieved = retrieveNext();
            if (retrieved == nullptr) {
                renderingTree->poll();
            } else {
                renderingTree->pushAndPoll(retrieved);
            }
            break;
        } else {
            #if defined(VERBOSEL2)
			traceprintf ("%s removed\n", rowToString(rendered, recordSize).c_str());
			#endif
        }
        lastRow = output;
	}
	
	return output;
}

tuple<vector<u_int8_t>, vector<u_int64_t>> parseDeviceType(string filename) {
    vector<u_int8_t> deviceTypes;
    vector<u_int64_t> switchPoints;
    std::regex deviceTypePattern("device(\\d+)");
    auto deviceTypeBegin = std::sregex_iterator(filename.begin(), filename.end(), deviceTypePattern);
    auto deviceTypeEnd = std::sregex_iterator();
    for (std::sregex_iterator i = deviceTypeBegin; i != deviceTypeEnd; ++i) {
        std::smatch match = *i;
        if (match.size() > 1) {
            u_int8_t deviceType = std::stoi(match[1].str());  // Access the first capturing group
            deviceTypes.push_back(deviceType);
        }
    }
    std::regex switchPointPattern("device\\d+-(\\d+)-");
    auto switchPointBegin = std::sregex_iterator(filename.begin(), filename.end(), switchPointPattern);
    auto switchPointEnd = std::sregex_iterator();
    for (std::sregex_iterator i = switchPointBegin; i != switchPointEnd; ++i) {
        std::smatch match = *i;
        if (match.size() > 1) {
            u_int64_t switchPoint = std::stoull(match[1].str());  // Access the first capturing group
            switchPoints.push_back(switchPoint);
        }
    }
    Assert(deviceTypes.size() == switchPoints.size() + 1, __FILE__, __LINE__);
    
    return std::make_tuple(deviceTypes, switchPoints);
}

u_int8_t getLargestDeviceType(string filename) {
    vector<u_int8_t> deviceTypes;
    vector<u_int64_t> switchPoints;
    std::tie(deviceTypes, switchPoints) = parseDeviceType(filename);
    if (deviceTypes.size() > 1) {
        std::cerr << "Warning: Multiple device types found in " << filename << ". You are only getting the largest device.\n";
    }
    return *std::max_element(deviceTypes.begin(), deviceTypes.end());
}