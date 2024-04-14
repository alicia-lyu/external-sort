#include "Buffer.h"

Buffer::Buffer (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (false);
    _rows = (byte *) malloc(size * count * sizeof(byte));
    _read = _rows;
    _filled = _rows;
    _engine.seed(_device());
}

Buffer::~Buffer ()
{
    TRACE (false);
    free(_rows);
}

byte * Buffer::fillRandomly ()
{
    if (_filled >= _rows + size * count) {
        traceprintf("Buffer is full, overwriting the first row\n");
        _filled = _rows;
    }
    std::generate(_filled, _filled + size, std::ref(_engine));
    _filled += size;
    return _filled - size;
}

byte * Buffer::copy (byte const * source)
{
    if (_filled >= _rows + size * count) {
        traceprintf("Buffer is full.\n");
        _filled = _rows;
        return nullptr;
    }
    std::copy(source, source + size, _filled);
    _filled += size;
    return _filled - size;
}

byte * Buffer::next ()
{
    if (_read >= _rows + size * count || _read > _filled) { 
        // If reaches the end of the buffer, return nullptr and reset _read to the beginning of the buffer.
        _read = _rows;
        return nullptr;
    }
    _read += size;
    return _read - size;
}

byte * Buffer::batchFillByOverwrite (u_int64_t toBeFilled)
{
    if (toBeFilled > size * count) {
        throw std::runtime_error("Buffer overflow");
    } else if (toBeFilled < size * count) {
        traceprintf("Buffer under-filled.\n");
    }
    _filled = _rows + toBeFilled;
    _read = _rows;
    return _rows;
}