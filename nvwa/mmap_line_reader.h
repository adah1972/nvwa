// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2016-2017 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  mmap_line_reader.h
 *
 * Header file for mmap_line_reader and mmap_line_reader_sv, easy-to-use
 * line-based file readers.  It is implemented with memory-mapped file APIs.
 *
 * @date  2017-09-10
 */

#ifndef NVWA_MMAP_LINE_READER_H
#define NVWA_MMAP_LINE_READER_H

#include <assert.h>             // assert
#include <stddef.h>             // ptrdiff_t/size_t
#include <iterator>             // std::input_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // NVWA_USES_CXX17/_DELETED/_NULLPTR
#include "mmap_reader_base.h"   // nvwa::mmap_reader_base

#include <string>               // std::string
#if NVWA_USES_CXX17
#include <string_view>          // std::string_view
#endif

NVWA_NAMESPACE_BEGIN

/** Class template to allow iteration over all lines of a mmappable file. */
template <typename _Tp>
class basic_mmap_line_reader : public mmap_reader_base
{
public:
    /** Iterator that contains the line content. */
    class iterator  // implements InputIterator
    {
    public:
        typedef _Tp                     value_type;
        typedef value_type*             pointer_type;
        typedef value_type&             reference;
        typedef ptrdiff_t               difference_type;
        typedef std::input_iterator_tag iterator_category;

        iterator() : _M_reader(_NULLPTR) {}
        explicit iterator(basic_mmap_line_reader* reader)
            : _M_reader(reader) , _M_offset(0) {}

        reference operator*()
        {
            assert(_M_reader != _NULLPTR);
            return _M_line;
        }
        pointer_type operator->()
        {
            assert(_M_reader != _NULLPTR);
            return &_M_line;
        }
        iterator& operator++()
        {
            if (!_M_reader->read(_M_line, _M_offset))
                _M_reader = _NULLPTR;
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }

        bool operator==(const iterator& rhs) const
        {
            return _M_reader == rhs._M_reader;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        basic_mmap_line_reader* _M_reader;
        size_t                  _M_offset;
        value_type              _M_line;
    };

    /** Enumeration of whether the delimiter should be stripped. */
    enum strip_type
    {
        strip_delimiter,     ///< The delimiter should be stripped
        no_strip_delimiter,  ///< The delimiter should be retained
    };

    explicit basic_mmap_line_reader(const char* path,
                                    char        delimiter = '\n',
                                    strip_type  strip = strip_delimiter)
        : mmap_reader_base(path)
        , _M_delimiter(delimiter)
        , _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#if NVWA_WINDOWS
    explicit basic_mmap_line_reader(const wchar_t* path,
                                    char           delimiter = '\n',
                                    strip_type     strip = strip_delimiter)
        : mmap_reader_base(path)
        , _M_delimiter(delimiter)
        , _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#endif
#if NVWA_UNIX
    explicit basic_mmap_line_reader(int        fd,
                                    char       delimiter = '\n',
                                    strip_type strip = strip_delimiter)
        : mmap_reader_base(fd)
        , _M_delimiter(delimiter)
        , _M_strip_delimiter(strip == strip_delimiter)
    {
    }
#endif
    ~basic_mmap_line_reader()
    {
    }

    iterator begin() { return iterator(this); }
    iterator end() const { return iterator(); }

    bool read(_Tp& output, size_t& offset);

private:
    basic_mmap_line_reader(const basic_mmap_line_reader&) _DELETED;
    basic_mmap_line_reader& operator=(const basic_mmap_line_reader&) _DELETED;

    char  _M_delimiter;
    bool  _M_strip_delimiter;
};

/**
 * Reads content from the mmaped file.
 *
 * @param[out]    output  object to receive the line
 * @param[in,out] offset  offset of reading pos on entry; end offset on exit
 * @return                \c true if line content is returned; \c false
 *                        otherwise
 */
template <typename _Tp>
bool basic_mmap_line_reader<_Tp>::read(_Tp& output, size_t& offset)
{
    if (offset == _M_size)
        return false;

    size_t pos = offset;
    bool found_delimiter = false;
    while (pos < _M_size)
    {
        char ch = _M_mmap_ptr[pos++];
        if (ch == _M_delimiter)
        {
            found_delimiter = true;
            break;
        }
    }

    output = _Tp(_M_mmap_ptr + offset,
                 pos - offset - (found_delimiter && _M_strip_delimiter));
    offset = pos;
    return true;
}

typedef basic_mmap_line_reader<std::string>      mmap_line_reader;
#if NVWA_USES_CXX17
typedef basic_mmap_line_reader<std::string_view> mmap_line_reader_sv;
#endif

NVWA_NAMESPACE_END

#endif // NVWA_MMAP_LINE_READER_H
