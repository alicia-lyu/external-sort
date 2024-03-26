#include "Iterator.h"
#include "defs.h"

typedef uint64_t ByteCount;

class DataPlan : public Plan
{
    friend class DataIterator;
public:
    DataPlan (ByteCount const byteCount);
    ~DataPlan ();
    Iterator * init () const;
private:
    ByteCount const _byteCount;
};

class DataIterator : public Iterator
{
public:
    DataIterator (DataPlan const * const plan);
    ~DataIterator ();
    bool next();
    byte getByte() const;
private:
    DataPlan const * const _plan;
    ByteCount _count;
};