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
 * @file  file_line_reader.cpp
 *
 * Code for file_line_reader, an easy-to-use line-based file reader.
 *
 * @date  2016-10-10
 */

#include <string.h>             // memcpy
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NOEXCEPT/_NULLPTR
#include "file_line_reader.h"   // file_line_reader

#if NVWA_CXX11_MODE
#include <utility>              // std::swap
#else
#include <algorithm>            // std::swap
#endif

NVWA_NAMESPACE_BEGIN

/** Size of buffer. */
const size_t BUFFER_SIZE = 256;

/**
 * Constructs the beginning iterator.
 *
 * @param reader  pointer to the file_line_reader object
 */
file_line_reader::iterator::iterator(file_line_reader* reader)
    : reader_(reader), size_(0)
{
    line_ = new char[BUFFER_SIZE];
    capacity_ = BUFFER_SIZE;
    ++*this;
}

/** Destuctor. */
file_line_reader::iterator::~iterator()
{
    delete[] line_;
}

/**
 * Copy-constructor.  The line content will be copied to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to copy from
 */
file_line_reader::iterator::iterator(const iterator& rhs)
    : reader_(rhs.reader_)
{
    line_ = new char[rhs.size_ + 1];
    size_ = rhs.size_;
    capacity_ = size_ + 1;
    memcpy(line_, rhs.line_, size_ + 1);
}

/**
 * Assignment.  The line content will be copied to the newly constructed
 * iterator.
 *
 * @param rhs  the iterator to copy from
 */
file_line_reader::iterator& file_line_reader::iterator::
operator=(const iterator& rhs)
{
    iterator temp(rhs);
    swap(temp);
    return *this;
}

#if HAVE_CXX11_RVALUE_REFERENCE

/**
 * Move constructor. The line content will be moved to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to move from
 */
file_line_reader::iterator::iterator(iterator&& rhs) _NOEXCEPT
    : reader_(rhs.reader_),
      line_(rhs.line_),
      size_(rhs.size_),
      capacity_(rhs.capacity_)
{
    rhs.reader_ = _NULLPTR;
    rhs.line_ = _NULLPTR;
    rhs.size_ = 0;
    rhs.capacity_ = 0;
}

/**
 * Move assignment. The line content will be moved to the newly
 * constructed iterator.
 *
 * @param rhs  the iterator to move from
 */
file_line_reader::iterator& file_line_reader::iterator::
operator=(iterator&& rhs) _NOEXCEPT
{
    iterator temp(std::move(rhs));
    swap(temp);
    return *this;
}

#endif // HAVE_CXX11_RVALUE_REFERENCE

/**
 * Swaps the iterator with another.
 *
 * @param rhs  the iterator to swap with
 */
void file_line_reader::iterator::swap(
    file_line_reader::iterator& rhs) _NOEXCEPT
{
    std::swap(reader_, rhs.reader_);
    std::swap(line_, rhs.line_);
    std::swap(size_, rhs.size_);
    std::swap(capacity_, rhs.capacity_);
}

/**
 * Constructor.
 *
 * @param stream     the file stream to read from
 * @param delimiter  the delimiter between text `lines' (default to LF)
 */
file_line_reader::file_line_reader(FILE* stream, char delimiter,
                                   strip_type strip)
    : stream_(stream)
    , delimiter_(delimiter)
    , strip_delimiter_(strip == strip_delimiter)
    , read_pos_(0)
    , size_(0)
{
    if (delimiter == '\n') {
        buffer_ = _NULLPTR;
    } else {
        buffer_ = new char[BUFFER_SIZE];
    }
}

/** Destructor. */
file_line_reader::~file_line_reader()
{
    delete[] buffer_;
}

static char* expand(char* data, size_t size, size_t capacity)
{
    char* new_ptr = new char[capacity];
    memcpy(new_ptr, data, size);
    delete[] data;
    return new_ptr;
}

/**
 * Reads content from the file stream.  If necessary, the receiving
 * buffer will be expanded so that it is big enough to contain all the
 * line content.
 *
 * @param[in,out] output    initial receiving buffer
 * @param[out]    size      size of the line
 * @param[in,out] capacity  capacity of the initial receiving buffer on
 *                          entering the function; it can be increased
 *                          when necessary
 * @return                  \c true if line content is returned;
 *                          \c false otherwise
 */
bool file_line_reader::read(char*& output, size_t& size, size_t& capacity)
{
    bool found_delimiter = false;
    size_t write_pos = 0;

    if (delimiter_ == '\n')
    {
        while (!found_delimiter)
        {
            if (!fgets(output + write_pos, capacity - write_pos, stream_))
                break;
            while (output[write_pos] != '\0' && output[write_pos] != '\n')
                ++write_pos;
            if (output[write_pos] == '\n')
            {
                ++write_pos;
                found_delimiter = true;
            }
            if (write_pos + 1 == capacity)
            {
                output = expand(output, write_pos, capacity * 2);
                capacity *= 2;
            }
        }
    }
    else
    {
        while (!found_delimiter)
        {
            if (read_pos_ == size_)
            {
                read_pos_ = 0;
                size_ = fread(buffer_, 1, sizeof buffer_, stream_);
                if (size_ == 0)
                    break;
            }
            char ch = buffer_[read_pos_++];
            if (write_pos + 1 == capacity)
            {
                output = expand(output, write_pos, capacity * 2);
                capacity *= 2;
            }
            output[write_pos++] = ch;
            if (ch == delimiter_)
                found_delimiter = true;
        }
    }

    if (write_pos != 0)
    {
        if (found_delimiter && strip_delimiter_)
            --write_pos;
        output[write_pos] = '\0';
        size = write_pos;
        return true;
    }
    else
    {
        return false;
    }
}

NVWA_NAMESPACE_END
