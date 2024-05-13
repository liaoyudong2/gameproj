//
// Created by liao on 2024/5/13.
//
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "buffer/Bio.h"

namespace Lcc {
    unsigned int _roundup_pow_of_2(unsigned int x) {
        unsigned int y = 0;
        for (int i = 0; i < 32; ++i) {
            y = static_cast<unsigned int>(1UL << i);
            if (x <= y) {
                break;
            }
        }
        return y;
    }

    BufferBio::BufferBio(): _in(0), _out(0), _size(0), _chunk(nullptr) {
    }

    BufferBio::~BufferBio() {
        if (_chunk) {
            ::free(_chunk);
        }
    }

    void BufferBio::Clear() {
        _in = _out = 0;
    }

    char *BufferBio::Data() const {
        return reinterpret_cast<char *>(_chunk + (_out & (_size - 1)));
    }

    unsigned int BufferBio::Capacity() const {
        return _size;
    }

    unsigned int BufferBio::UsedSize() const {
        return _in - _out;
    }

    unsigned int BufferBio::AvailSize() const {
        return _size - (_in - _out);
    }

    unsigned int BufferBio::Read(char *out, unsigned int size) {
        size = std::min(size, _in - _out);
        unsigned int l = std::min(size, _size - (_out & (_size - 1)));
        memcpy(out, _chunk + (_out & (_size - 1)), l);
        memcpy(reinterpret_cast<unsigned char *>(out + l), _chunk, size - l);
        _out += size;
        return size;
    }

    unsigned int BufferBio::Write(const char *in, unsigned int size) {
        const unsigned int avail = _size - (_in - _out);
        if (avail < size) {
            unsigned int l = _roundup_pow_of_2(_size + (size - avail));
            if (l > 0x80000000UL) {
                return 0;
            }
            auto *chunk = static_cast<unsigned char *>(::realloc(_chunk, l));
            if (!chunk) {
                return 0;
            }
            _size = l;
            _chunk = chunk;
            return Write(in, size);
        }
        unsigned int l = std::min(size, _size - (_in & (_size - 1)));
        memcpy(_chunk + (_in & (_size - 1)), in, l);
        memcpy(_chunk, reinterpret_cast<const unsigned char *>(in + l), size - l);
        _in += size;
        return size;
    }
}
