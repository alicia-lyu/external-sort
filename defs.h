#pragma once

#include <stddef.h>
#include <stdint.h>
#include <cstdio>
#include <string>
#include <filesystem>
#include <cmath>
#include <map>

typedef unsigned char byte;
typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: byte
using string = std::string;
using std::to_string;
using std::map;
#define SEPARATOR std::filesystem::path::preferred_separator

#define slotsof(a)	(sizeof (a) / sizeof (a[0]))

// #define nullptr	((void *) NULL)

#define yesno(b)	((b) ? "yes" : "no")

// call-through to assert() from <assert.h>
//
void Assert (bool const predicate,
		char const * const file, int const line);
//
#if defined ( _DEBUG )  ||  defined (DEBUG)
#define DebugAssert(b)	Assert ((b), __FILE__, __LINE__)
#else // _DEBUG DEBUG
#define DebugAssert(b)	(void) (0)
#endif // _DEBUG DEBUG
//
#define FinalAssert(b)	Assert ((b), __FILE__, __LINE__)
//	#define FinalAssert(b)	(void) (0)
//
#define ParamAssert(b)	FinalAssert (b)

#define traceprintf printf ("%s:%d:%s ", \
		__FILE__, __LINE__, __FUNCTION__), \
	printf

// -----------------------------------------------------------------

#define OP_STATE 0
#define OP_ACCESS 1
#define OP_RESULT 2
#define MERGE_RUNS_HDD 3
#define MERGE_RUNS_SSD 4
#define MERGE_RUNS_BOTH 5
#define SORT_MINI_RUNS 6
#define SPILL_RUNS_SSD 7
#define SPILL_RUNS_HDD 8
#define READ_RUN_PAGES_SSD 9
#define READ_RUN_PAGES_HDD 10
#define INIT_SORT 11
#define SORT_RESULT 12
#define VERIFY_RESULT 13
#define WITNESS_RESULT 14
#define METRICS_RESULT 15

// used to record access to storage devices
#define STORAGE_READ 16
#define STORAGE_WRITE 17


class Trace
{
public :

	Trace (bool const trace, char const * const function,
			char const * const file, int const line);
	~Trace ();

	static void PrintTrace(int opType, const string & message);
	static void PrintTrace(int opType, int subOpType, const string & message);
	static string FormatSize(u_int64_t size);

private :

	void _trace (char const lead []);

	bool const _output;
	char const * const _function;
	char const * const _file;
	int const _line;

	static map<int, string> opName;
}; // class Trace

#define TRACE(trace)	Trace __trace (trace, __FUNCTION__, __FILE__, __LINE__)

// -----------------------------------------------------------------

template <class Value> inline bool odd (
		Value const value, int bits_from_lsb = 0)
{
	return ((value >> bits_from_lsb) & Value (1)) != 0;
	// Alternatively `((Value (1) << bits_from_lsb) & value) != 0`
	// as symmetrical to `even`
}
//
template <class Value> inline bool even (
		Value const value, int bits_from_lsb = 0)
{
	return ((Value (1) << bits_from_lsb) & value) == 0;
}

// templates for generic min and max operations
//
template <class Val> inline Val min (Val const a, Val const b)
{
	return a < b ? a : b;
}
//
template <class Val> inline Val max (Val const a, Val const b)
{
	return a > b ? a : b;
}
//
template <class Val> inline Val between (Val const value, Val const low, Val const high)
	// 'value' but no less than 'low' and no more than 'high'
{
	return (value < low ? low : value > high ? high : value);
}
//
template <class Val> inline void extremes (Val const value, Val & low, Val & high)
	// retain 'value' in 'low' or 'high' keeping track of extremes
{
	if (value < low) low = value;
	if (value > high) high = value;
}

// templates for generic division
// should work for any integer type
//
template <class Val> inline Val divide (Val const a, Val const b)
{
	return 1 + (a - 1) / b;
} // divide
//
template <class Val> inline Val roundup (Val const a, Val const b)
{
	return b * divide (a, b);
} // roundup

template <class Val> inline void exchange (Val & a, Val & b)
{
	Val const c = a;
	a = b;
	b = c;
} // exchange

// -----------------------------------------------------------------

template <class Val> inline Val mask (int const from, int const to)
{
	// from: index of lsb set to 1, range 0..31
	// to: index of msb set to 1, range from..31
	// from==to ==> single bit set
	//
	return (((Val) 2) << to) - (((Val) 1) << from);
} // Value mask (int from, int to)

size_t Random (size_t const range);
size_t Random (size_t const low_incl, size_t const high_incl);
size_t RoundDown (size_t const x, size_t const y);
size_t RoundUp (size_t const x, size_t const y);
bool IsPowerOf2 (size_t const x);
size_t lsb (size_t const x);
size_t msb (size_t const x);
int msbi (size_t const x);
char const * YesNo (bool const b);
char const * OkBad (bool const b);
