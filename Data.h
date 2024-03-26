#include <random>

#include "Iterator.h"
#include "defs.h"

typedef uint64_t ByteCount;

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

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
    byte _byte;
    random_bytes_engine engine;
};