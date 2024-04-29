#include <stdlib.h>
#include <stdio.h>

#include "defs.h"

// -----------------------------------------------------------------

Trace::Trace (bool const trace, char const * const function,
		char const * const file, int const line)
	: _output (trace), _function (function), _file (file), _line (line)
{
	_trace (">>>>>");
} // Trace::Trace

Trace::~Trace ()
{
	_trace ("<<<<<");
} // Trace::~Trace

void Trace::_trace (char const lead [])
{
	if (_output)
		printf ("%s %s (%s:%d)\n", lead, _function, _file, _line);
} // Trace::_trace

map<int, string> Trace::opName = {
	{OP_STATE, "STATE"},
	{OP_ACCESS, "ACCESS"},
	{OP_RESULT, "RESULT"},
	{MERGE_RUNS_HDD, "MERGE_RUNS_HDD"},
	{MERGE_RUNS_SSD, "MERGE_RUNS_SSD"},
	{MERGE_RUNS_BOTH, "MERGE_RUNS_BOTH"},
	{SORT_MINI_RUNS, "SORT_MINI_RUNS"},
	{SPILL_RUNS_SSD, "SPILL_RUNS_SSD"},
	{SPILL_RUNS_HDD, "SPILL_RUNS_HDD"},
	{READ_RUN_PAGES_SSD, "READ_RUN_PAGES_SSD"},
	{READ_RUN_PAGES_HDD, "READ_RUN_PAGES_HDD"},
	{INIT_SORT, "INIT_SORT"},
	{SORT_RESULT, "SORT"},
	{VERIFY_RESULT, "VERIFY"},
	{WITNESS_RESULT, "WITNESS"},
	{METRICS_RESULT, "METRICS"}
};

int Trace::lastOp = -1;

void Trace::PrintTrace(int opType, const string & message)
{
	if (lastOp == OP_ACCESS && opType != OP_ACCESS) {
		FlushAccess();
	}

	printf ("[%s] -> %s\n", opName[opType].c_str(), message.c_str());
	lastOp = opType;
}

void Trace::PrintTrace(int opType, int subOpType, const string & message)
{
	if (lastOp == OP_ACCESS && opType != OP_ACCESS) {
		FlushAccess();
	}

	printf ("[%s] -> [%s]: %s\n", opName[opType].c_str(), opName[subOpType].c_str(), message.c_str());
	lastOp = opType;
}

map<tuple<int, int>, tuple<double, u_int64_t, u_int64_t>> Trace::accessTrace = {
	{{ACCESS_READ, STORAGE_SSD}, {0.0, 0, 0}},
	{{ACCESS_READ, STORAGE_HDD}, {0.0, 0, 0}},
	{{ACCESS_WRITE, STORAGE_SSD}, {0.0, 0, 0}},
	{{ACCESS_WRITE, STORAGE_HDD}, {0.0, 0, 0}}
};

void Trace::TraceAccess(int accessOp, int deviceType, double latency, u_int64_t numBytes)
{
	auto key = make_tuple(accessOp, deviceType);
	auto it = accessTrace.find(key);
	if (it == accessTrace.end()) {
		accessTrace[key] = make_tuple(latency, numBytes, 1);
	} else {
		auto & [totalLatency, totalBytes, totalAccess] = it->second;
		totalLatency += latency;
		totalBytes += numBytes;
		totalAccess += 1;
	}
	lastOp = OP_ACCESS;
}

void Trace::FlushAccess()
{
	// enumerate all pairs of (accessOp, deviceType)
	for (auto & [key, value] : accessTrace) {
		auto & [latency, numBytes, numAccess] = value;
		if (numBytes > 0) {
			WriteAccess(std::get<0>(key), std::get<1>(key), latency, numBytes, numAccess);
			latency = 0.0;
			numBytes = 0;
		}
	}
}

void Trace::WriteAccess(int accessOp, int deviceType, double latency, u_int64_t numBytes, u_int64_t numAccess)
{
	string output = "A total of " + to_string(numAccess);;
	output += (accessOp == ACCESS_READ) ? " reads" : " writes";
	output += " to ";
	output += getDeviceName(deviceType);
	output += " device were made with ";
	output += to_string(numBytes) + " bytes ";
	if (numBytes > 1000) {
		output += "(" + FormatSize(numBytes) + ") ";
	}
	output += "and latency " + to_string(int(latency*1e6)) + " us";
	PrintTrace(OP_ACCESS, output);
}

string Trace::FormatSize(u_int64_t size)
{
	string unit = "B";
	double size_d = size;
	if (size_d >= 1000) {
		size_d /= 1000;
		unit = "KB";
	}
	if (size_d >= 1000) {
		size_d /= 1000;
		unit = "MB";
	}
	if (size_d >= 1000) {
		size_d /= 1000;
		unit = "GB";
	}

	char buffer[100];

	// if is an integer, don't show decimal places
	if (size_d == (int) size_d) {
		snprintf(buffer, 99, "%d ", (int) size_d);
	} else {
		snprintf(buffer, 99, "%.2f ", size_d);
	}

	return string(buffer) + unit;
}

// -----------------------------------------------------------------

size_t Random (size_t const range)
{
	return (size_t) rand () % range;
} // Random

size_t Random (size_t const low_incl, size_t const high_incl)
{
	return low_incl + (size_t) rand () % (high_incl - low_incl + 1);
} // Random

size_t RoundDown (size_t const x, size_t const y)
{
	return x - (x % y);
} // RoundDown

size_t RoundUp (size_t const x, size_t const y)
{
	size_t const z = x % y;
	return (z == 0 ? x : x + y - z);
} // RoundUp

bool IsPowerOf2 (size_t const x)
{
	return x > 0 && (x & (x - 1)) == 0;
} // IsPowerOf2

size_t lsb (size_t const x)
{
	size_t const y = x & (x - 1);
	return x ^ y;
} // lsb

size_t msb (size_t const x)
{
	size_t y = x;
	for (size_t z;  z = y & (y - 1), z != 0;  y = z)
		; // nothing
	return y;
} // msb

int msbi (size_t const x)
{
	int i = 0;
	for (size_t z = 2;  z <= x;  ++ i, z <<= 1)
		; // nothing
	return i;
} // msbi

char const * YesNo (bool const b)
{
	return b ? "Yes" : "No";
} // YesNo

char const * OkBad (bool const b)
{
	return b ? "Ok" : "Bad";
} // OkBad
