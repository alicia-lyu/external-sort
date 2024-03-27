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
	row_ptr getRow();
private:
	RowCount _count;
	row_ptr _row;
}; // class Iterator
