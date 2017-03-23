// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file  istream_line_reader.h
 *
 * Header file for istream_line_reader, an easy-to-use line-based
 * istream reader.
 *
 * This class allows using istreams in a Pythonic way (if your compiler
 * supports C++11 or later), e.g.:
 * @code
 * for (auto& line : nvwa::istream_line_reader(std::cin)) {
 *     // Process line
 * }
 * @endcode
 *
 * This code can be used without C++11, but using it would be less
 * convenient then.
 *
 * It was first published in a <a href="https://yongweiwu.wordpress.com/2016/08/16/python-yield-and-cplusplus-coroutines/">blog</a>,
 * and has since been modified to satisfy the \c InputIterator concept,
 * along with other minor changes.
 *
 * @date  2017-03-23
 */

#ifndef NVWA_ISTREAM_LINE_READER_H
#define NVWA_ISTREAM_LINE_READER_H

#include <assert.h>             // assert
#include <istream>              // std::istream
#include <iterator>             // std::input_iterator_tag
#include <string>               // std::string
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NOEXCEPT/_NULLPTR

NVWA_NAMESPACE_BEGIN

/** Class to allow iteration over all lines from an input stream. */
class istream_line_reader {
public:
    /**
     * Iterator that contains the line content.
     *
     * The iterator \e owns the content.
     */
    class iterator  // implements InputIterator
    {
    public:
        typedef int                     difference_type;
        typedef std::string             value_type;
        typedef value_type*             pointer_type;
        typedef value_type&             reference;
        typedef std::input_iterator_tag iterator_category;

        iterator() : _M_stream(_NULLPTR) {}
        explicit iterator(std::istream& is) : _M_stream(&is)
        {
            ++*this;
        }

        reference operator*()
        {
            assert(_M_stream != _NULLPTR);
            return _M_line;
        }
        pointer_type operator->()
        {
            assert(_M_stream != _NULLPTR);
            return &_M_line;
        }
        iterator& operator++()
        {
            assert(_M_stream != _NULLPTR);
            getline(*_M_stream, _M_line);
            if (!*_M_stream)
                _M_stream = _NULLPTR;
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
            return _M_stream == rhs._M_stream;
        }
        bool operator!=(const iterator& rhs) const
        {
            return !operator==(rhs);
        }

    private:
        std::istream* _M_stream;
        std::string   _M_line;
    };

    explicit istream_line_reader(std::istream& is) _NOEXCEPT
        : _M_stream(is)
    {
    }
    iterator begin()
    {
        return iterator(_M_stream);
    }
    iterator end() const
    {
        return iterator();
    }

private:
    std::istream& _M_stream;
};

NVWA_NAMESPACE_END

#endif // NVWA_ISTREAM_LINE_READER_H
