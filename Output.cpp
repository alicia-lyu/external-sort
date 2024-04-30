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

    _lastRow = new byte[_recordSize+5];
} // OutputPrinter::OutputPrinter

OutputPrinter::~OutputPrinter ()
{
    answerFile.close();
    outputFile.close();
    delete[] _lastRow;
} // OutputPrinter::~OutputPrinter

void OutputPrinter::Print()
{
    while (!answerFile.eof()) {
        answerFile.read(reinterpret_cast<char *>(_lastRow), _recordSize);
        if (answerFile.eof()) {
            break;
        }
        outputFile.write(reinterpret_cast<char *>(_lastRow), _recordSize);
        outputFile.write("\n", 1);
    }
} // OutputPrinter::Print