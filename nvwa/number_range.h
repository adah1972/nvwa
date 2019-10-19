// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2019 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  number_range.h
 *
 * Header file for number_range, a number range type that satisfies the
 * InputRange concept.  A compiler that supports C++17 or later is
 * required.
 *
 * @date  2019-10-19
 */

#ifndef NVWA_NUMBER_RANGE_H
#define NVWA_NUMBER_RANGE_H

#include <stddef.h>             // ptrdiff_t
#include <iterator>             // std::input_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

/**
 * Class template that allows iterating over a number range with a
 * step value other than one.  It is quite similar to the C++20 \c
 * iota_view, except for allowing non-integer types and custom step
 * values.  It satisfies the InputRange concept, and can work with
 * std::ranges and range-v3.
 */
template <typename _Tp>
class number_range {
public:
    class sentinel;

    class iterator {  // implements InputIterator
    public:
        typedef ptrdiff_t               difference_type;
        typedef _Tp                     value_type;
        typedef value_type*             pointer;
        typedef value_type&             reference;
        typedef std::input_iterator_tag iterator_category;

        iterator() : _M_curr(0), _M_step(0) {}
        iterator(_Tp val, _Tp step) : _M_curr(val), _M_step(step) {}
        iterator& operator++()
        {
            _M_curr += _M_step;
            return *this;
        }
        iterator operator++(int)
        {
            iterator temp(*this);
            ++*this;
            return temp;
        }
        value_type operator*() const
        {
            return _M_curr;
        }

        friend class sentinel;
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
        bool operator==(const sentinel& rhs) const;
        bool operator!=(const sentinel& rhs) const;

    private:
        _Tp _M_curr;
        _Tp _M_step;
    };

    class sentinel {
    public:
        sentinel() : _M_end(0) {}
        explicit sentinel(_Tp end) : _M_end(end) {}

        friend class iterator;
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;
        bool operator==(const sentinel& rhs) const;
        bool operator!=(const sentinel& rhs) const;

    private:
        _Tp _M_end;
    };

    number_range() : _M_begin(0), _M_end(0), _M_step(0) {}
    number_range(_Tp begin, _Tp end, _Tp step = 1)
        : _M_begin(begin)
        , _M_end(end)
        , _M_step(step) {}

    iterator begin() const
    {
        return iterator(_M_begin, _M_step);
    }

    sentinel end() const
    {
        return sentinel(_M_end);
    }

private:
    _Tp _M_begin;
    _Tp _M_end;
    _Tp _M_step;
};

template <typename _Tp>
inline bool number_range<_Tp>::iterator::operator==(const iterator& rhs) const
{
    return _M_curr == rhs._M_curr;
}

template <typename _Tp>
inline bool number_range<_Tp>::iterator::operator!=(const iterator& rhs) const
{
    return !operator==(rhs);
}

template <typename _Tp>
inline bool number_range<_Tp>::iterator::operator==(const sentinel& rhs) const
{
    return _M_curr >= rhs._M_end;
}

template <typename _Tp>
inline bool number_range<_Tp>::iterator::operator!=(const sentinel& rhs) const
{
    return !operator==(rhs);
}

template <typename _Tp>
inline bool number_range<_Tp>::sentinel::operator==(const iterator& rhs) const
{
    return rhs._M_curr >= _M_end;
}

template <typename _Tp>
inline bool number_range<_Tp>::sentinel::operator!=(const iterator& rhs) const
{
    return !operator==(rhs);
}

template <typename _Tp>
inline bool number_range<_Tp>::sentinel::
            operator==(const sentinel& /*rhs*/) const
{
    return true;
}

template <typename _Tp>
inline bool number_range<_Tp>::sentinel::
            operator!=(const sentinel& /*rhs*/) const
{
    return false;
}

NVWA_NAMESPACE_END

#endif // NVWA_NUMBER_RANGE_H
