#pragma once

#include "defs.h"

typedef uint64_t RowCount;
typedef u_int16_t RowSize; // 20-2000, unit: byte

class Plan
{
	friend class Iterator;
public:
	Plan ();
	virtual ~Plan ();
	virtual class Iterator * init () const = 0;
private:
}; // class Plan

class Iterator
{
public:
	Iterator ();
	virtual ~Iterator ();
	void run ();
	virtual bool next () = 0;
private:
	RowCount _count;
}; // class Iterator
