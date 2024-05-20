// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017-2024 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  istream_line_reader.h
 *
 * Header file for istream_line_reader, an easy-to-use line-based
 * istream reader.
 *
 * This class allows using istreams in a Pythonic way, e.g.:
 * @code
 * for (auto& line : nvwa::istream_line_reader(std::cin)) {
 *     // Process line
 * }
 * @endcode
 *
 * It was first published in a <a href="https://yongweiwu.wordpress.com/2016/08/16/python-yield-and-cplusplus-coroutines/">blog</a>,
 * and has since been modified to make its iterator satisfy the
 * \c InputIterator concept, along with other minor changes.
 *
 * @date  2024-05-20
 */

#ifndef NVWA_ISTREAM_LINE_READER_H
#define NVWA_ISTREAM_LINE_READER_H

#include <assert.h>             // assert
#include <stddef.h>             // ptrdiff_t
#include <istream>              // std::istream
#include <iterator>             // std::input_iterator_tag
#include <stdexcept>            // std::runtime_error
#include <string>               // std::string
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // HAVE_CXX20_RANGES

NVWA_NAMESPACE_BEGIN

/** Class to allow iteration over all lines from an input stream. */
class istream_line_reader {
public:
    /**
     * Iterator that contains the line content.
     *
     * The iterator \e owns the content.
     */
    class iterator {  // implements InputIterator
    public:
        typedef ptrdiff_t               difference_type;
        typedef std::string             value_type;
        typedef const value_type*       pointer;
        typedef const value_type&       reference;
        typedef std::input_iterator_tag iterator_category;

        iterator() = default;
        explicit iterator(std::istream& is) : _M_stream(&is)
        {
            ++*this;
        }

        reference operator*() const noexcept
        {
            assert(_M_stream != nullptr);
            return _M_line;
        }
        pointer operator->() const noexcept
        {
            assert(_M_stream != nullptr);
            return &_M_line;
        }
        iterator& operator++()
        {
            assert(_M_stream != nullptr);
            getline(*_M_stream, _M_line);
            if (!*_M_stream) {
                _M_stream = nullptr;
            }
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }

        bool operator==(const iterator& rhs) const noexcept
        {
            // This implementation basically says, any iterators
            // pointing to the same stream are equal.  This behaviour
            // may seem a little surprising in the beginning, but, in
            // reality, it hardly has any consequences, as people
            // usually compare an input iterator only to the sentinel
            // object.  The alternative, using _M_stream->tellg() to
            // get the exact position, harms the performance too dearly.
            // I do not really have a better choice.
            //
            // In fact, C++20 has removed the requirement that input
            // iterators be equality comparable, although I have to
            // support it, both for comparison with the "sentinel"
            // (default-constructed iterator here) and for compatibility
            // with earlier C++ standards (the Cpp17InputIterator
            // requirement).
            //
            // If you do need to compare valid iterators, consider using
            // file_line_reader or mmap_line_reader.
            return _M_stream == rhs._M_stream;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !operator==(rhs);
        }

    private:
        std::istream* _M_stream{};
        std::string   _M_line;
    };

    explicit istream_line_reader(std::istream& is) noexcept : _M_stream(&is)
    {
    }
    iterator begin()
    {
        if (_M_stream->fail()) {
            throw std::runtime_error("input stream error");
        }
        return iterator(*_M_stream);
    }
    iterator end() const noexcept
    {
        return {};
    }

private:
    std::istream* _M_stream;
};

NVWA_NAMESPACE_END

#if HAVE_CXX20_RANGES
#include <ranges>

template <>
inline constexpr bool
    std::ranges::enable_borrowed_range<NVWA::istream_line_reader> = true;
#endif

#endif // NVWA_ISTREAM_LINE_READER_H
