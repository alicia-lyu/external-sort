#include "Data.h"

Buffer::Buffer (u_int16_t count, RowSize size):
    count (count), size (size)
{
    TRACE (true);
    _rows = (byte *) malloc(size * count * sizeof(byte));
    _filled = _rows;
    _engine.seed(_device());
}

Buffer::~Buffer ()
{
    TRACE (true);
    free(_rows);
}

byte * Buffer::fillRandomly ()
{
    if (_filled >= _rows + size * count) {
        _filled = _rows;
    }
    std::generate(_filled, _filled + size, std::ref(_engine));
    _filled += size;
    return _filled - size;
}

byte * Buffer::copy (byte const * source)
{
    if (_filled >= _rows + size * count) {
        _filled = _rows;
    }
    std::copy(source, source + size, _filled);
    _filled += size;
    return _filled - size;
}

byte * Buffer::next ()
{
    if (_read > _filled) {
        throw std::runtime_error("Attempted to read buffer space not filled yet.");
    }
    if (_read >= _rows + size * count) {
        _read = _rows;
    }
    _read += size;
    return _read - size;
}

byte * Buffer::data ()
{
    return _rows;
}