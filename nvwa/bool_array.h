// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2010 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file    bool_array.h
 *
 * Header file for class bool_array (packed boolean array).
 *
 * @version 3.11, 2010/10/16
 * @author  Wu Yongwei
 *
 */

#ifndef _BOOL_ARRAY_H
#define _BOOL_ARRAY_H

#include <assert.h>     // assert
#include <stdlib.h>     // exit, free, and NULL
#include <new>          // std::bad_alloc
#include <stdexcept>    // std::out_of_range
#include <string>       // for exception constructors

/**
 * Class to represent a packed boolean array.
 *
 * This was first written in April 1995, before I knew of any existing
 * implementation of this kind of classes.  Of course, the C++ Standard
 * Template Library now demands an implementation of packed boolean
 * array as \c vector&lt;bool&gt;, but the code here should still be
 * useful for the following reasons:
 *
 *  -# Some compilers (like MSVC 6) did not implement this
 *     specialization (and they may not have a \c bit_vector either);
 *  -# I included some additional member functions, like \e #initialize,
 *     \e #count, and \e #find, which should be useful;
 *  -# My tests show that the code here is significantly FASTER
 *     than \c vector&lt;bool&gt; (and the normal boolean array)
 *     under MSVC versions 6/8/9 and GCC versions before 4.3 (while
 *     the \c vector&lt;bool&gt; implementations of MSVC 7.1 and
 *     GCC 4.3 have performance similar to that of \c bool_array).
 */
class bool_array
{
public:
#if (defined(__x86_64) || defined(__ia64) || defined(__ppc64__) || \
     defined(_WIN64) || defined(_M_IA64)) && \
    !(defined(_LP64) || defined(__lp64))
    /** Type of array indices. */
    typedef unsigned long long  size_type;
#else
    /** Type of array indices. */
    typedef unsigned long       size_type;
#endif

private:
    /** Private definition of byte to avoid polluting the global namespace. */
    typedef unsigned char       byte;

    /** Class to represent a reference to an array element. */
    template <typename _Byte_type>
    class _Element
    {
    public:
        _Element(_Byte_type* pointer, size_type index);
        bool operator=(bool value);
        operator bool() const;
    private:
        _Byte_type* _M_byte_ptr;
        size_t      _M_byte_idx;
        size_t      _M_bit_idx;
    };

public:
    typedef _Element<byte> reference;
    typedef _Element<const byte> const_reference;

#if defined(_MSC_VER) && _MSC_VER < 1300
    enum { npos = (size_type)-1  /**< Constant representing `not found' */ };
#else
    /** Constant representing `not found'. */
    static const size_type npos = (size_type)-1;
#endif

    bool_array() : _M_byte_ptr(NULL), _M_length(0) {}
    explicit bool_array(size_type size);
    ~bool_array() { if (_M_byte_ptr != NULL) free(_M_byte_ptr); }

    bool_array(const bool_array& rhs);
    bool_array& operator=(const bool_array& rhs);

    bool create(size_type size);
    void initialize(bool value);

    reference operator[](size_type index);
    const_reference operator[](size_type index) const;

    bool at(size_type index) const;
    void reset(size_type index);
    void set(size_type index);

    size_type size() const { return _M_length; }
    size_type count() const;
    size_type count(size_type begin, size_type end = npos) const;
    size_type find(bool value, size_type offset = 0) const;
    size_type find(bool value, size_type offset, size_type count) const;
    size_type find_until(bool value, size_type begin, size_type end) const;

    void flip();
    void swap(bool_array& rhs);
    void merge_and(const bool_array& rhs,
                   size_type begin = 0,
                   size_type end = npos,
                   size_type offset = 0);
    void merge_or (const bool_array& rhs,
                   size_type begin = 0,
                   size_type end = npos,
                   size_type offset = 0);

private:
    byte get_8bits(size_type offset, size_type end) const;

    byte*           _M_byte_ptr;
    size_type       _M_length;
    static byte     _S_bit_count[256];
    static byte     _S_bit_ordinal[256];
};


/* Inline functions */

/**
 * Constructs a reference to an array element.
 *
 * @param pointer  pointer to the interal boolean data
 * @param index    index of the array element to access
 */
template <typename _Byte_type>
inline bool_array::_Element<_Byte_type>::_Element(
        _Byte_type* pointer,
        size_type index)
{
    _M_byte_ptr = pointer;
    _M_byte_idx = (size_t)(index / 8);
    _M_bit_idx  = (size_t)(index % 8);
}

/**
 * Assigns a new boolean value to an array element.
 *
 * @param value  the new boolean value
 * @return       the assigned boolean value
 */
template <typename _Byte_type>
inline bool bool_array::_Element<_Byte_type>::operator=(bool value)
{
    if (value)
        *(_M_byte_ptr + _M_byte_idx) |= 1 << _M_bit_idx;
    else
        *(_M_byte_ptr + _M_byte_idx) &= ~(1 << _M_bit_idx);
    return value;
}

/**
 * Reads the boolean value from an array element.
 *
 * @return  the boolean value of the accessed array element
 */
template <typename _Byte_type>
inline bool_array::_Element<_Byte_type>::operator bool() const
{
    return *(_M_byte_ptr + _M_byte_idx) & (1 << _M_bit_idx) ? true : false;
}

/**
 * Creates a reference to an array element.
 *
 * @param index index of the array element to access
 * @return      reference to the specified element
 */
inline bool_array::reference bool_array::operator[](size_type index)
{
    assert(_M_byte_ptr);
    assert(index < _M_length);
    return reference(_M_byte_ptr, index);
}

/**
 * Creates a const reference to an array element.
 *
 * @param index  index of the array element to access
 * @return       const reference to the specified element
 */
inline bool_array::const_reference bool_array::operator[](size_type index)const
{
    assert(_M_byte_ptr);
    assert(index < _M_length);
    return const_reference(_M_byte_ptr, index);
}

/**
 * Reads the boolean value of an array element via an index.
 *
 * @param index         index of the array element to access
 * @return              the boolean value of the accessed array element
 * @throw out_of_range  when the index is too big
 */
inline bool bool_array::at(size_type index) const
{
    size_t byte_idx, bit_idx;
    if (index >= _M_length)
        throw std::out_of_range("invalid bool_array index");
    byte_idx = (size_t)(index / 8);
    bit_idx  = (size_t)(index % 8);
    return *(_M_byte_ptr + byte_idx) & (1 << bit_idx) ? true : false;
}

/**
 * Resets an array element to \c false via an index.
 *
 * @param index         index of the array element to access
 * @throw out_of_range  when the index is too big
 */
inline void bool_array::reset(size_type index)
{
    size_t byte_idx, bit_idx;
    if (index >= _M_length)
        throw std::out_of_range("invalid bool_array index");
    byte_idx = (size_t)(index / 8);
    bit_idx  = (size_t)(index % 8);
    *(_M_byte_ptr + byte_idx) &= ~(1 << bit_idx);
}

/**
 * Sets an array element to \c true via an index.
 *
 * @param index         index of the array element to access
 * @throw out_of_range  when the index is too big
 */
inline void bool_array::set(size_type index)
{
    size_t byte_idx, bit_idx;
    if (index >= _M_length)
        throw std::out_of_range("invalid bool_array index");
    byte_idx = (size_t)(index / 8);
    bit_idx  = (size_t)(index % 8);
    *(_M_byte_ptr + byte_idx) |= 1 << bit_idx;
}

/**
 * Searches for the specified boolean value.  This function seaches from
 * the specified position (default to beginning) to the end.
 *
 * @param offset  index of the position at which the search is to begin
 * @param value   the boolean value to find
 * @return        index of the first value found if successful; \c #npos
 *                otherwise
 */
inline bool_array::size_type bool_array::find(
        bool value,
        size_type offset) const
{
    return find_until(value, offset, _M_length);
}

/**
 * Searches for the specified boolean value.  This function accepts a
 * range expressed in {position, count}.
 *
 * @param offset        index of the position at which the search is to begin
 * @param count         the number of bits to search
 * @param value         the boolean value to find
 * @return              index of the first value found if successful; \c #npos
 *                      otherwise
 * @throw out_of_range  \a offset and/or \a count is too big
 */
inline bool_array::size_type bool_array::find(
        bool value,
        size_type offset,
        size_type count) const
{
    return find_until(value, offset, offset + count);
}

#endif // _BOOL_ARRAY_H
