// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017 Wu Yongwei <wuyongwei at gmail dot com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute
 * it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must
 *    not claim that you wrote the original software.  If you use this
 *    software in a product, an acknowledgement in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must
 *    not be misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 *
 * This file is part of Stones of Nvwa:
 *      https://github.com/adah1972/nvwa
 *
 */

/**
 * @file  mmap_byte_reader.h
 *
 * Header file for mmap_byte_reader, an easy-to-use byte-based file reader.
 * It is implemented with memory-mapped file APIs.
 *
 * @date  2017-09-12
 */

#ifndef NVWA_MMAP_BYTE_READER_H
#define NVWA_MMAP_BYTE_READER_H

#include <stddef.h>             // ptrdiff_t/size_t
#include <iterator>             // std::random_access_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _DELETED
#include "mmap_reader_base.h"   // nvwa::mmap_reader_base

NVWA_NAMESPACE_BEGIN

/** Class template to allow iteration over all bytes of a mmappable file. */
template <typename _Tp>
class basic_mmap_byte_reader : public mmap_reader_base
{
public:
    typedef _Tp                value_type;
    typedef const value_type*  pointer_type;
    typedef const value_type&  reference;
    typedef const value_type&  const_reference;

    /** Iterator over the bytes. */
    class iterator  // implements RandomAccessIterator
    {
    public:
        typedef _Tp                             value_type;
        typedef const value_type*               pointer_type;
        typedef const value_type&               reference;
        typedef const value_type&               const_reference;
        typedef ptrdiff_t                       difference_type;
        typedef std::random_access_iterator_tag iterator_category;

        explicit iterator(const basic_mmap_byte_reader* reader,
                          size_t offset = 0)
            : _M_reader(reader)
            , _M_offset(offset)
        {
        }

        reference operator*()
        {
            return _M_reader->get(_M_offset);
        }
        pointer_type operator->()
        {
            return &_M_reader->get(_M_offset);
        }
        iterator& operator++()
        {
            ++_M_offset;
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }
        iterator& operator--()
        {
            --_M_offset;
            return *this;
        }
        iterator operator--(int)
        {
            iterator temp(*this);
            --*this;
            return temp;
        }
        iterator& operator+=(difference_type i)
        {
            _M_offset += i;
            return *this;
        }
        iterator& operator-=(difference_type i)
        {
            _M_offset -= i;
            return *this;
        }
        iterator operator+(difference_type i) const
        {
            return iterator(_M_reader, _M_offset + i);
        }
        iterator operator-(difference_type i) const
        {
            return iterator(_M_reader, _M_offset - i);
        }
        difference_type operator-(const iterator& rhs) const
        {
            return _M_offset - rhs._M_offset;
        }
        reference operator[](difference_type i) const
        {
            return _M_reader->get(_M_offset + i);
        }

        bool operator==(const iterator& rhs) const
        {
            return _M_reader == rhs._M_reader && _M_offset == rhs._M_offset;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }
        bool operator<(const iterator& rhs) const
        {
            return _M_reader < rhs._M_reader ||
                   (_M_reader == rhs._M_reader &&
                    _M_offset < rhs._M_offset);
        }
        bool operator>(const iterator& rhs) const
        {
            return _M_reader > rhs._M_reader ||
                   (_M_reader == rhs._M_reader &&
                    _M_offset > rhs._M_offset);
        }
        bool operator<=(const iterator& rhs) const
        {
            return !operator>(rhs);
        }
        bool operator>=(const iterator& rhs) const
        {
            return !operator<(rhs);
        }

    private:
        const basic_mmap_byte_reader* _M_reader;
        size_t                        _M_offset;
    };

    typedef iterator const_iterator;

    explicit basic_mmap_byte_reader(const char* path)
        : mmap_reader_base(path)
    {
    }
#if NVWA_WINDOWS
    explicit basic_mmap_byte_reader(const wchar_t* path)
        : mmap_reader_base(path)
    {
    }
#endif
#if NVWA_UNIX
    explicit basic_mmap_byte_reader(int fd)
        : mmap_reader_base(fd)
    {
    }
#endif
    ~basic_mmap_byte_reader()
    {
    }

    iterator begin() const { return iterator(this); }
    iterator end() const { return iterator(this, _M_size); }

    reference get(size_t offset) const
    {
        return reinterpret_cast<reference>(_M_mmap_ptr[offset]);
    }

private:
    basic_mmap_byte_reader(const basic_mmap_byte_reader&) _DELETED;
    basic_mmap_byte_reader& operator=(const basic_mmap_byte_reader&) _DELETED;
};

typedef basic_mmap_byte_reader<char>          mmap_char_reader;
typedef basic_mmap_byte_reader<unsigned char> mmap_byte_reader;

NVWA_NAMESPACE_END

#endif // NVWA_MMAP_BYTE_READER_H
