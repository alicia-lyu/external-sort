#include "Iterator.h"

Plan::Plan ()
{
	TRACE (false);
} // Plan::Plan

Plan::~Plan ()
{
	TRACE (false);
} // Plan::~Plan

Iterator::Iterator () : _count (0)
{
	TRACE (false);
} // Iterator::Iterator

Iterator::~Iterator ()
{
	TRACE (false);
} // Iterator::~Iterator

void Iterator::run ()
{
	TRACE (false);

	while (next ())  ++ _count;

	traceprintf ("entire plan produced %lu rows\n",
			(unsigned long) _count);
} // Iterator::run