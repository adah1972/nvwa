// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004 Wu Yongwei <adah at users dot sourceforge dot net>
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
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
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
 * @file    bool_array.h
 *
 * Header file for class bool_array (packed boolean array).
 *
 * @version 2.0, 2004/05/09
 * @author  Wu Yongwei
 *
 */

#ifndef _BOOL_ARRAY_H
#define _BOOL_ARRAY_H

#ifndef _BYTE_DEFINED
#define _BYTE_DEFINED
typedef unsigned char BYTE;
#endif // !_BYTE_DEFINED

#include <assert.h>     // assert
#include <stdlib.h>     // exit, free, and NULL
#include <new>          // std::bad_alloc
#include <stdexcept>    // std::out_of_range
#include <string>       // for exception constructors

/**
 * Class to represent a packed boolean array.
 *
 * This was first written in April 1995, before I knew any existing
 * implmentation of this kind of classes.  Of course, now the C++
 * Standard Template Library has a good implementation of packed boolean
 * array as `vector<bool>',  but the code here should still be useful
 * for the following three reasons: (1) STL support of MSVC 6 did not
 * implement this specialization (nor did it have a `bit_vector'); (2) I
 * incorporated some useful member functions from the STL bitset into
 * this `bool_array', including `reset', `set', `flip', and `count';
 * (3) In my tests under MSVC 6 and GCC 2.95.3/3.2.3 my code is really
 * FASTER than vector<bool> or the normal boolean array.
 */
class bool_array
{
    /** Class to represent a reference to an array element. */
    class _Element
    {
    public:
        _Element(BYTE* __ptr, unsigned long __idx);
        bool operator=(bool __value);
        operator bool() const;
    private:
        BYTE*   _M_byte_ptr;
        size_t  _M_byte_idx;
        size_t  _M_bit_idx;
    };

public:
    bool_array() : _M_byte_ptr(NULL), _M_length(0) {}
    explicit bool_array(unsigned long __size);
    ~bool_array() { if (_M_byte_ptr != NULL) free(_M_byte_ptr); }

    bool create(unsigned long __size);
    void initialize(bool __value);

    // Using unsigned type here can increase performance!
    _Element operator[](unsigned long __idx);
    bool at(unsigned long __idx) const;
    void reset(unsigned long __idx);
    void set(unsigned long __idx);

    unsigned long size() const { return _M_length; }
    unsigned long count() const;
    unsigned long count(unsigned long __i1, unsigned long __i2) const;
    void flip();

private:
    BYTE*           _M_byte_ptr;
    unsigned long   _M_length;
    static BYTE     _S_bit_count[256];
};


/* Inline functions */

inline bool_array::_Element::_Element(BYTE* __ptr, unsigned long __idx)
{
    _M_byte_ptr = __ptr;
    _M_byte_idx = (size_t)(__idx / 8);
    _M_bit_idx  = (size_t)(__idx % 8);
}

inline bool bool_array::_Element::operator=(bool __value)
{
    if (__value)
        *(_M_byte_ptr + _M_byte_idx) |= 1 << _M_bit_idx;
    else
        *(_M_byte_ptr + _M_byte_idx) &= ~(1 << _M_bit_idx);
    return __value;
}

inline bool_array::_Element::operator bool() const
{
    return *(_M_byte_ptr + _M_byte_idx) & (1 << _M_bit_idx) ? true : false;
}

inline bool_array::bool_array(unsigned long __size)
    : _M_byte_ptr(NULL), _M_length(0)
{
    if (__size == 0)
        throw std::out_of_range("invalid bool_array size");

    if (!create(__size))
        throw std::bad_alloc();
}

inline bool_array::_Element bool_array::operator[](unsigned long __idx)
{
    assert(_M_byte_ptr);
    assert(__idx < _M_length);
    return _Element(_M_byte_ptr, __idx);
}

inline bool bool_array::at(unsigned long __idx) const
{
    size_t __byte_idx, __bit_idx;
    if (__idx >= _M_length)
        throw std::out_of_range("invalid bool_array subscript");
    __byte_idx = (size_t)(__idx / 8);
    __bit_idx  = (size_t)(__idx % 8);
    return *(_M_byte_ptr + __byte_idx) & (1 << __bit_idx) ? true : false;
}

inline void bool_array::reset(unsigned long __idx)
{
    size_t __byte_idx, __bit_idx;
    if (__idx >= _M_length)
        throw std::out_of_range("invalid bool_array subscript");
    __byte_idx = (size_t)(__idx / 8);
    __bit_idx  = (size_t)(__idx % 8);
    *(_M_byte_ptr + __byte_idx) &= ~(1 << __bit_idx);
}

inline void bool_array::set(unsigned long __idx)
{
    size_t __byte_idx, __bit_idx;
    if (__idx >= _M_length)
        throw std::out_of_range("invalid bool_array subscript");
    __byte_idx = (size_t)(__idx / 8);
    __bit_idx  = (size_t)(__idx % 8);
    *(_M_byte_ptr + __byte_idx) |= 1 << __bit_idx;
}

#endif // _BOOL_ARRAY_H
