#pragma once

#include "defs.h"
#include "Row.h"

typedef uint64_t RowCount;

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
	Row * getRow();
private:
	RowCount _count;
	Row * _row;
}; // class Iterator
