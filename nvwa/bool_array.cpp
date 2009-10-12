// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2009 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file    bool_array.cpp
 *
 * Code for class bool_array (packed boolean array).
 *
 * @version 3.6, 2009/10/12
 * @author  Wu Yongwei
 *
 */

#include <limits.h>         // UINT_MAX, ULONG_MAX
#include <string.h>         // memset
#include <algorithm>        // std::swap
#include "bool_array.h"     // bool_array
#include "static_assert.h"  // STATIC_ASSERT

/**
 * Array that contains pre-calculated values how many 1-bits there are
 * in a given byte.
 */
BYTE bool_array::_S_bit_count[256] =
{
    0, /*   0 */ 1, /*   1 */ 1, /*   2 */ 2, /*   3 */ 1, /*   4 */
    2, /*   5 */ 2, /*   6 */ 3, /*   7 */ 1, /*   8 */ 2, /*   9 */
    2, /*  10 */ 3, /*  11 */ 2, /*  12 */ 3, /*  13 */ 3, /*  14 */
    4, /*  15 */ 1, /*  16 */ 2, /*  17 */ 2, /*  18 */ 3, /*  19 */
    2, /*  20 */ 3, /*  21 */ 3, /*  22 */ 4, /*  23 */ 2, /*  24 */
    3, /*  25 */ 3, /*  26 */ 4, /*  27 */ 3, /*  28 */ 4, /*  29 */
    4, /*  30 */ 5, /*  31 */ 1, /*  32 */ 2, /*  33 */ 2, /*  34 */
    3, /*  35 */ 2, /*  36 */ 3, /*  37 */ 3, /*  38 */ 4, /*  39 */
    2, /*  40 */ 3, /*  41 */ 3, /*  42 */ 4, /*  43 */ 3, /*  44 */
    4, /*  45 */ 4, /*  46 */ 5, /*  47 */ 2, /*  48 */ 3, /*  49 */
    3, /*  50 */ 4, /*  51 */ 3, /*  52 */ 4, /*  53 */ 4, /*  54 */
    5, /*  55 */ 3, /*  56 */ 4, /*  57 */ 4, /*  58 */ 5, /*  59 */
    4, /*  60 */ 5, /*  61 */ 5, /*  62 */ 6, /*  63 */ 1, /*  64 */
    2, /*  65 */ 2, /*  66 */ 3, /*  67 */ 2, /*  68 */ 3, /*  69 */
    3, /*  70 */ 4, /*  71 */ 2, /*  72 */ 3, /*  73 */ 3, /*  74 */
    4, /*  75 */ 3, /*  76 */ 4, /*  77 */ 4, /*  78 */ 5, /*  79 */
    2, /*  80 */ 3, /*  81 */ 3, /*  82 */ 4, /*  83 */ 3, /*  84 */
    4, /*  85 */ 4, /*  86 */ 5, /*  87 */ 3, /*  88 */ 4, /*  89 */
    4, /*  90 */ 5, /*  91 */ 4, /*  92 */ 5, /*  93 */ 5, /*  94 */
    6, /*  95 */ 2, /*  96 */ 3, /*  97 */ 3, /*  98 */ 4, /*  99 */
    3, /* 100 */ 4, /* 101 */ 4, /* 102 */ 5, /* 103 */ 3, /* 104 */
    4, /* 105 */ 4, /* 106 */ 5, /* 107 */ 4, /* 108 */ 5, /* 109 */
    5, /* 110 */ 6, /* 111 */ 3, /* 112 */ 4, /* 113 */ 4, /* 114 */
    5, /* 115 */ 4, /* 116 */ 5, /* 117 */ 5, /* 118 */ 6, /* 119 */
    4, /* 120 */ 5, /* 121 */ 5, /* 122 */ 6, /* 123 */ 5, /* 124 */
    6, /* 125 */ 6, /* 126 */ 7, /* 127 */ 1, /* 128 */ 2, /* 129 */
    2, /* 130 */ 3, /* 131 */ 2, /* 132 */ 3, /* 133 */ 3, /* 134 */
    4, /* 135 */ 2, /* 136 */ 3, /* 137 */ 3, /* 138 */ 4, /* 139 */
    3, /* 140 */ 4, /* 141 */ 4, /* 142 */ 5, /* 143 */ 2, /* 144 */
    3, /* 145 */ 3, /* 146 */ 4, /* 147 */ 3, /* 148 */ 4, /* 149 */
    4, /* 150 */ 5, /* 151 */ 3, /* 152 */ 4, /* 153 */ 4, /* 154 */
    5, /* 155 */ 4, /* 156 */ 5, /* 157 */ 5, /* 158 */ 6, /* 159 */
    2, /* 160 */ 3, /* 161 */ 3, /* 162 */ 4, /* 163 */ 3, /* 164 */
    4, /* 165 */ 4, /* 166 */ 5, /* 167 */ 3, /* 168 */ 4, /* 169 */
    4, /* 170 */ 5, /* 171 */ 4, /* 172 */ 5, /* 173 */ 5, /* 174 */
    6, /* 175 */ 3, /* 176 */ 4, /* 177 */ 4, /* 178 */ 5, /* 179 */
    4, /* 180 */ 5, /* 181 */ 5, /* 182 */ 6, /* 183 */ 4, /* 184 */
    5, /* 185 */ 5, /* 186 */ 6, /* 187 */ 5, /* 188 */ 6, /* 189 */
    6, /* 190 */ 7, /* 191 */ 2, /* 192 */ 3, /* 193 */ 3, /* 194 */
    4, /* 195 */ 3, /* 196 */ 4, /* 197 */ 4, /* 198 */ 5, /* 199 */
    3, /* 200 */ 4, /* 201 */ 4, /* 202 */ 5, /* 203 */ 4, /* 204 */
    5, /* 205 */ 5, /* 206 */ 6, /* 207 */ 3, /* 208 */ 4, /* 209 */
    4, /* 210 */ 5, /* 211 */ 4, /* 212 */ 5, /* 213 */ 5, /* 214 */
    6, /* 215 */ 4, /* 216 */ 5, /* 217 */ 5, /* 218 */ 6, /* 219 */
    5, /* 220 */ 6, /* 221 */ 6, /* 222 */ 7, /* 223 */ 3, /* 224 */
    4, /* 225 */ 4, /* 226 */ 5, /* 227 */ 4, /* 228 */ 5, /* 229 */
    5, /* 230 */ 6, /* 231 */ 4, /* 232 */ 5, /* 233 */ 5, /* 234 */
    6, /* 235 */ 5, /* 236 */ 6, /* 237 */ 6, /* 238 */ 7, /* 239 */
    4, /* 240 */ 5, /* 241 */ 5, /* 242 */ 6, /* 243 */ 5, /* 244 */
    6, /* 245 */ 6, /* 246 */ 7, /* 247 */ 5, /* 248 */ 6, /* 249 */
    6, /* 250 */ 7, /* 251 */ 6, /* 252 */ 7, /* 253 */ 7, /* 254 */
    8  /* 255 */
}; // End _S_bit_count

/**
 * Array that contains pre-calculated values which the first 1-bit is
 * for a given byte.  The first element indicates an invalid value
 * (there are only 0-bits).
 */
BYTE bool_array::_S_bit_ordinal[256] =
{
    9, /*   0 */
    0, /*   1 */ 1, /*   2 */ 0, /*   3 */ 2, /*   4 */ 0, /*   5 */
    1, /*   6 */ 0, /*   7 */ 3, /*   8 */ 0, /*   9 */ 1, /*  10 */
    0, /*  11 */ 2, /*  12 */ 0, /*  13 */ 1, /*  14 */ 0, /*  15 */
    4, /*  16 */ 0, /*  17 */ 1, /*  18 */ 0, /*  19 */ 2, /*  20 */
    0, /*  21 */ 1, /*  22 */ 0, /*  23 */ 3, /*  24 */ 0, /*  25 */
    1, /*  26 */ 0, /*  27 */ 2, /*  28 */ 0, /*  29 */ 1, /*  30 */
    0, /*  31 */ 5, /*  32 */ 0, /*  33 */ 1, /*  34 */ 0, /*  35 */
    2, /*  36 */ 0, /*  37 */ 1, /*  38 */ 0, /*  39 */ 3, /*  40 */
    0, /*  41 */ 1, /*  42 */ 0, /*  43 */ 2, /*  44 */ 0, /*  45 */
    1, /*  46 */ 0, /*  47 */ 4, /*  48 */ 0, /*  49 */ 1, /*  50 */
    0, /*  51 */ 2, /*  52 */ 0, /*  53 */ 1, /*  54 */ 0, /*  55 */
    3, /*  56 */ 0, /*  57 */ 1, /*  58 */ 0, /*  59 */ 2, /*  60 */
    0, /*  61 */ 1, /*  62 */ 0, /*  63 */ 6, /*  64 */ 0, /*  65 */
    1, /*  66 */ 0, /*  67 */ 2, /*  68 */ 0, /*  69 */ 1, /*  70 */
    0, /*  71 */ 3, /*  72 */ 0, /*  73 */ 1, /*  74 */ 0, /*  75 */
    2, /*  76 */ 0, /*  77 */ 1, /*  78 */ 0, /*  79 */ 4, /*  80 */
    0, /*  81 */ 1, /*  82 */ 0, /*  83 */ 2, /*  84 */ 0, /*  85 */
    1, /*  86 */ 0, /*  87 */ 3, /*  88 */ 0, /*  89 */ 1, /*  90 */
    0, /*  91 */ 2, /*  92 */ 0, /*  93 */ 1, /*  94 */ 0, /*  95 */
    5, /*  96 */ 0, /*  97 */ 1, /*  98 */ 0, /*  99 */ 2, /* 100 */
    0, /* 101 */ 1, /* 102 */ 0, /* 103 */ 3, /* 104 */ 0, /* 105 */
    1, /* 106 */ 0, /* 107 */ 2, /* 108 */ 0, /* 109 */ 1, /* 110 */
    0, /* 111 */ 4, /* 112 */ 0, /* 113 */ 1, /* 114 */ 0, /* 115 */
    2, /* 116 */ 0, /* 117 */ 1, /* 118 */ 0, /* 119 */ 3, /* 120 */
    0, /* 121 */ 1, /* 122 */ 0, /* 123 */ 2, /* 124 */ 0, /* 125 */
    1, /* 126 */ 0, /* 127 */ 7, /* 128 */ 0, /* 129 */ 1, /* 130 */
    0, /* 131 */ 2, /* 132 */ 0, /* 133 */ 1, /* 134 */ 0, /* 135 */
    3, /* 136 */ 0, /* 137 */ 1, /* 138 */ 0, /* 139 */ 2, /* 140 */
    0, /* 141 */ 1, /* 142 */ 0, /* 143 */ 4, /* 144 */ 0, /* 145 */
    1, /* 146 */ 0, /* 147 */ 2, /* 148 */ 0, /* 149 */ 1, /* 150 */
    0, /* 151 */ 3, /* 152 */ 0, /* 153 */ 1, /* 154 */ 0, /* 155 */
    2, /* 156 */ 0, /* 157 */ 1, /* 158 */ 0, /* 159 */ 5, /* 160 */
    0, /* 161 */ 1, /* 162 */ 0, /* 163 */ 2, /* 164 */ 0, /* 165 */
    1, /* 166 */ 0, /* 167 */ 3, /* 168 */ 0, /* 169 */ 1, /* 170 */
    0, /* 171 */ 2, /* 172 */ 0, /* 173 */ 1, /* 174 */ 0, /* 175 */
    4, /* 176 */ 0, /* 177 */ 1, /* 178 */ 0, /* 179 */ 2, /* 180 */
    0, /* 181 */ 1, /* 182 */ 0, /* 183 */ 3, /* 184 */ 0, /* 185 */
    1, /* 186 */ 0, /* 187 */ 2, /* 188 */ 0, /* 189 */ 1, /* 190 */
    0, /* 191 */ 6, /* 192 */ 0, /* 193 */ 1, /* 194 */ 0, /* 195 */
    2, /* 196 */ 0, /* 197 */ 1, /* 198 */ 0, /* 199 */ 3, /* 200 */
    0, /* 201 */ 1, /* 202 */ 0, /* 203 */ 2, /* 204 */ 0, /* 205 */
    1, /* 206 */ 0, /* 207 */ 4, /* 208 */ 0, /* 209 */ 1, /* 210 */
    0, /* 211 */ 2, /* 212 */ 0, /* 213 */ 1, /* 214 */ 0, /* 215 */
    3, /* 216 */ 0, /* 217 */ 1, /* 218 */ 0, /* 219 */ 2, /* 220 */
    0, /* 221 */ 1, /* 222 */ 0, /* 223 */ 5, /* 224 */ 0, /* 225 */
    1, /* 226 */ 0, /* 227 */ 2, /* 228 */ 0, /* 229 */ 1, /* 230 */
    0, /* 231 */ 3, /* 232 */ 0, /* 233 */ 1, /* 234 */ 0, /* 235 */
    2, /* 236 */ 0, /* 237 */ 1, /* 238 */ 0, /* 239 */ 4, /* 240 */
    0, /* 241 */ 1, /* 242 */ 0, /* 243 */ 2, /* 244 */ 0, /* 245 */
    1, /* 246 */ 0, /* 247 */ 3, /* 248 */ 0, /* 249 */ 1, /* 250 */
    0, /* 251 */ 2, /* 252 */ 0, /* 253 */ 1, /* 254 */ 0  /* 255 */
}; // End _S_bit_ordinal

/**
 * Constructs the packed boolean array with a specific size.
 *
 * @param __size            size of the array
 * @throw std::out_of_range if \a __size equals \c 0
 * @throw std::bad_alloc    if memory is insufficient
 */
bool_array::bool_array(size_type __size)
    : _M_byte_ptr(NULL), _M_length(0)
{
    if (__size == 0)
        throw std::out_of_range("invalid bool_array size");

    if (!create(__size))
        throw std::bad_alloc();
}

/**
 * Creates the packed boolean array with a specific size.
 *
 * @param __size    size of the array
 * @return          \c false if \a __size equals \c 0 or is too big, or
 *                  if memory is insufficient; \c true if \a __size has
 *                  a suitable value and memory allocation is
 *                  successful.
 */
bool bool_array::create(size_type __size)
{
    if (__size == 0)
        return false;

#if defined(__x86_64) || defined(_WIN64) || defined(_M_IA64)
    STATIC_ASSERT(sizeof(size_t) == sizeof(size_type),  Wrong_size_type);
#else
    STATIC_ASSERT(sizeof(size_t) <= sizeof(size_type),  Wrong_size_type);
    STATIC_ASSERT(sizeof(size_t)==sizeof(unsigned int), Wrong_size_assumption);
    // Will be optimized away by a decent compiler if ULONG_MAX == UINT_MAX
    if (ULONG_MAX > UINT_MAX && ((__size - 1) / 8 + 1) > UINT_MAX)
        return false;
#endif

    size_t __byte_cnt = (size_t)((__size - 1) / 8 + 1);
    BYTE* __byte_ptr = (BYTE*)malloc(__byte_cnt);
    if (__byte_ptr == NULL)
        return false;

    if (_M_byte_ptr)
        free(_M_byte_ptr);

    _M_byte_ptr = __byte_ptr;
    _M_length = __size;
    return true;
}

/**
 * Initializes all array elements to a specific value optimally.
 *
 * @param __val   the boolean value to assign to all elements
 */
void bool_array::initialize(bool __val)
{
    assert(_M_byte_ptr);
    size_t __byte_cnt = (size_t)((_M_length - 1) / 8) + 1;
    memset(_M_byte_ptr, __val ? ~0 : 0, __byte_cnt);
    if (__val)
    {
        int __valid_bits_in_last_byte = (_M_length - 1) % 8 + 1;
        _M_byte_ptr[__byte_cnt - 1] &= ~(~0 << __valid_bits_in_last_byte);
    }
}

/**
 * Counts elements with a \c true value.
 *
 * @return  the count of \c true elements
 */
bool_array::size_type bool_array::count() const
{
    assert(_M_byte_ptr);
    size_type __true_cnt = 0;
    size_t __byte_cnt = (size_t)((_M_length - 1) / 8) + 1;
    for (size_t __i = 0; __i < __byte_cnt; ++__i)
        __true_cnt += _S_bit_count[_M_byte_ptr[__i]];
    return __true_cnt;
}

/**
 * Counts elements with a \c true value in a specified range.
 *
 * @param __beg beginning of the range
 * @param __end end of the range (exclusive)
 * @return      the count of \c true elements
 */
bool_array::size_type bool_array::count(size_type __beg, size_type __end) const
{
    assert(_M_byte_ptr);
    size_type __true_cnt = 0;
    size_t __byte_idx_beg, __byte_idx_end;
    BYTE __byte_val;

    if (__beg == __end)
        return 0;
    if (__beg > __end || __end > _M_length)
        throw std::out_of_range("invalid bool_array range");
    --__end;

    __byte_idx_beg = (size_t)(__beg / 8);
    __byte_val = _M_byte_ptr[__byte_idx_beg];
    __byte_val &= ~0 << (__beg % 8);

    __byte_idx_end = (size_t)(__end / 8);
    if (__byte_idx_beg < __byte_idx_end)
    {
        __true_cnt = _S_bit_count[__byte_val];
        __byte_val = _M_byte_ptr[__byte_idx_end];
    }
    __byte_val &= ~(~0 << (__end % 8 + 1));
    __true_cnt += _S_bit_count[__byte_val];

    for (++__byte_idx_beg; __byte_idx_beg < __byte_idx_end; ++__byte_idx_beg)
        __true_cnt += _S_bit_count[_M_byte_ptr[__byte_idx_beg]];
    return __true_cnt;
}

/**
 * Searches for the specified boolean value.  This function accepts a
 * range expressed in [beginning, end).
 *
 * @param __beg index of the position at which the search is to begin
 * @param __end index of the end position (exclusive) to stop searching
 * @param __val the boolean value to find
 * @return      index of the first value found if successful; \c #npos
 *              otherwise
 */
bool_array::size_type bool_array::find_until(
        bool __val,
        size_type __beg,
        size_type __end) const
{
    assert(_M_byte_ptr);

    if (__beg == __end)
        return npos;
    if (__beg > __end || __end > _M_length)
        throw std::out_of_range("invalid bool_array range");
    --__end;

    size_t __byte_idx_beg = (size_t)(__beg / 8);
    size_t __byte_idx_end = (size_t)(__end / 8);
    BYTE __byte_val = _M_byte_ptr[__byte_idx_beg];

    if (__val)
    {
        __byte_val &= ~0 << (__beg % 8);
        for (size_t __i = __byte_idx_beg; __i < __byte_idx_end;)
        {
            if (__byte_val != 0)
                return __i * 8 + _S_bit_ordinal[__byte_val];
            __byte_val = _M_byte_ptr[++__i];
        }
        __byte_val &= ~(~0 << (__end % 8 + 1));
        if (__byte_val != 0)
            return __byte_idx_end * 8 + _S_bit_ordinal[__byte_val];
    }
    else
    {
        __byte_val |= ~(~0 << (__beg % 8));
        for (size_t __i = __byte_idx_beg; __i < __byte_idx_end;)
        {
            if (__byte_val != 0xFF)
                return __i * 8 + _S_bit_ordinal[(BYTE)~__byte_val];
            __byte_val = _M_byte_ptr[++__i];
        }
        __byte_val |= ~0 << (__end % 8 + 1);
        if (__byte_val != 0xFF)
            return __byte_idx_end * 8 + _S_bit_ordinal[(BYTE)~__byte_val];
    }

    return npos;
}

/**
 * Changes all \c true elements to \c false, and \c false ones to \c true.
 */
void bool_array::flip()
{
    assert(_M_byte_ptr);
    size_t __byte_cnt = (size_t)((_M_length - 1) / 8) + 1;
    for (size_t __i = 0; __i < __byte_cnt; ++__i)
        _M_byte_ptr[__i] = ~_M_byte_ptr[__i];
    int __valid_bits_in_last_byte = (_M_length - 1) % 8 + 1;
    _M_byte_ptr[__byte_cnt - 1] &= ~(~0 << __valid_bits_in_last_byte);
}

/**
 * Exchanges the content of this bool_array with another.
 *
 * @param rhs   another bool_array to exchange content with
 */
void bool_array::swap(bool_array& rhs)
{
    std::swap(_M_byte_ptr, rhs._M_byte_ptr);
    std::swap(_M_length,   rhs._M_length);
}
