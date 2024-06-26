#pragma once

#include "params.h"
#include "defs.h"
#include "Buffer.h"

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
	virtual byte * next () = 0;
	virtual bool forceFlushBuffer ();
private:
	RowCount _count;
}; // class Iterator
