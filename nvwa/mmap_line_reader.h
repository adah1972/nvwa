// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2016-2017 Wu Yongwei <adah at users dot sourceforge dot net>
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
 *      http://sourceforge.net/projects/nvwa
 *
 */

/**
 * @file  mmap_line_reader.h
 *
 * Header file for mmap_line_reader, an easy-to-use line-based file reader.
 * It is implemented with the POSIX mmap API.
 *
 * @date  2017-03-23
 */

#ifndef NVWA_MMAP_LINE_READER_H
#define NVWA_MMAP_LINE_READER_H

#include <assert.h>             // assert
#include <unistd.h>             // off_t
#include <iterator>             // std::input_iterator_tag
#include <string>               // std::string
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NULLPTR

NVWA_NAMESPACE_BEGIN

/** Class to allow iteration over all lines of a mmappable file. */
class mmap_line_reader
{
public:
    /** Iterator that contains the line content. */
    class iterator  // implements InputIterator
    {
    public:
        typedef int                     difference_type;
        typedef std::string             value_type;
        typedef value_type*             pointer_type;
        typedef value_type&             reference;
        typedef std::input_iterator_tag iterator_category;

        iterator() : _M_reader(_NULLPTR) {}
        explicit iterator(mmap_line_reader* reader)
            : _M_reader(reader) , _M_offset(0) {}

        reference operator*()
        {
            assert(_M_reader != _NULLPTR);
            return _M_line;
        }
        value_type* operator->()
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
        mmap_line_reader* _M_reader;
        off_t             _M_offset;
        std::string       _M_line;
    };

    /** Enumeration of whether the delimiter should be stripped. */
    enum strip_type
    {
        strip_delimiter,     ///< The delimiter should be stripped
        no_strip_delimiter,  ///< The delimiter should be retained
    };

    explicit mmap_line_reader(const char* path, char delimiter = '\n',
                              strip_type strip = strip_delimiter);
    explicit mmap_line_reader(int fd, char delimiter = '\n',
                              strip_type strip = strip_delimiter);
    ~mmap_line_reader();

    iterator begin() { return iterator(this); }
    iterator end() const { return iterator(); }

    bool read(std::string& output, off_t& offset);

private:
    mmap_line_reader(const mmap_line_reader&) _DELETED;
    mmap_line_reader& operator=(const mmap_line_reader&) _DELETED;

    void initialize();

    int   _M_fd;
    char  _M_delimiter;
    bool  _M_strip_delimiter;
    char* _M_mmap_ptr;
    off_t _M_size;
};

NVWA_NAMESPACE_END

#endif // NVWA_MMAP_LINE_READER_H
