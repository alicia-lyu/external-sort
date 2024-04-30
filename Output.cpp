#include "Output.h"

OutputPrinter::OutputPrinter (const string &outputPath, const RowSize recordSize) :
    _recordSize (recordSize)
{
    auto &answerPath = Trace::finalOutputFileName;

    answerFile.open(answerPath, ios::binary);
    if (!answerFile.is_open()) {
        throw std::runtime_error("Sorted output file not found");
    }

    outputFile.open(outputPath, ios::binary);
    if (!outputFile.is_open()) {
        throw std::runtime_error("Cannot create output file");
    }

    // read as many bytes in one time as possible depending on the memory size
    bytesPerRead = MEMORY_SIZE / recordSize * recordSize;
    _lastRow = new byte[bytesPerRead + 5];
} // OutputPrinter::OutputPrinter

OutputPrinter::~OutputPrinter ()
{
    delete[] _lastRow;
} // OutputPrinter::~OutputPrinter

void OutputPrinter::Print()
{
    while (!answerFile.eof()) {
        answerFile.read(reinterpret_cast<char *>(_lastRow), _recordSize);
        bytesRead = answerFile.gcount();
        if (bytesRead == 0) {
            break;
        }

        for (u_int64_t i = 0; i < bytesRead; i += _recordSize) {
            outputFile.write(reinterpret_cast<char *>(_lastRow + i), _recordSize);
            outputFile.write("\n", 1);
        }
    }

    answerFile.close();
    outputFile.close();

    // remove the answer file
    remove(Trace::finalOutputFileName.c_str());
} // OutputPrinter::Print