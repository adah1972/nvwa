// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2016 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file  file_line_reader.h
 *
 * Header file for file_line_reader, an easy-to-use line-based file reader.
 *
 * @date  2016-10-12
 */

#ifndef FILE_LINE_READER_H
#define FILE_LINE_READER_H

#include <assert.h>             // assert
#include <stdio.h>              // file streams
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NOEXCEPT/_NULLPTR

NVWA_NAMESPACE_BEGIN

/** Class to allow iteration over all lines of a text file. */
class file_line_reader
{
public:
    /**
     * Iterator that contains the line content.
     *
     * The iterator \e owns the content, and the content does not contain
     * the delimiter.
     */
    class iterator  // implements InputIterator
    {
    public:
        typedef char*& reference;
        typedef char* value_type;

        iterator() _NOEXCEPT : reader_(_NULLPTR), line_(_NULLPTR) {}
        explicit iterator(file_line_reader* reader);
        ~iterator();

        iterator(const iterator& rhs);
        iterator& operator=(const iterator& rhs);

#if HAVE_CXX11_RVALUE_REFERENCE
        iterator(iterator&& rhs) _NOEXCEPT;
        iterator& operator=(iterator&& rhs) _NOEXCEPT;
#endif

        void swap(iterator& rhs) _NOEXCEPT;

        reference operator*()
        {
            assert(reader_ != _NULLPTR);
            return line_;
        }
        value_type* operator->()
        {
            assert(reader_ != _NULLPTR);
            return &line_;
        }
        iterator& operator++()
        {
            if (!reader_->read(line_, size_, capacity_))
                reader_ = _NULLPTR;
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
            return reader_ == rhs.reader_;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

        size_t size() const { return size_; }

    private:
        file_line_reader* reader_;
        char*             line_;
        size_t            size_;
        size_t            capacity_;
    };

    enum strip_type {
        strip_delimiter,
        no_strip_delimiter,
    };

    explicit file_line_reader(FILE* stream, char delimiter = '\n',
                              strip_type strip = strip_delimiter);
    ~file_line_reader();

    iterator begin() { return iterator(this); }
    iterator end() const _NOEXCEPT { return iterator(); }
    bool read(char*& output, size_t& size, size_t& capacity);

private:
    file_line_reader(const file_line_reader&) _DELETED;
    file_line_reader& operator=(const file_line_reader&) _DELETED;

    FILE*  stream_;
    char   delimiter_;
    bool   strip_delimiter_;
    char*  buffer_;
    size_t read_pos_;
    size_t size_;
};

inline void swap(file_line_reader::iterator& lhs,
                 file_line_reader::iterator& rhs)
{
    lhs.swap(rhs);
}

NVWA_NAMESPACE_END

#endif // FILE_LINE_READER_H
