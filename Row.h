#include "Iterator.h"
#include "defs.h"

class Row
{
public:
    RowSize size;
private:
    byte * _bytes;
public:
    Row (RowSize size);
    ~Row ();
    byte * begin();
    byte * end();
};