// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  split.h
 *
 * Header file for an efficient, lazy split function, when using ranges
 * seems an overkill.
 *
 * It allows using split in a Pythonic way, e.g.:
 * @code
 * for (auto& word : nvwa::split(a_long_string_or_string_view, ' ')) {
 *     // Process word
 * }
 * @endcode
 *
 * @date  2017-12-31
 */

#ifndef NVWA_SPLIT_H
#define NVWA_SPLIT_H

#include <assert.h>             // assert
#include <iterator>             // std::input_iterator_tag
#include <string>               // std::basic_string
#include <string_view>          // std::basic_string_view
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

/**
 * Class to allow iteration over split items from the input.
 *
 * @param _StringType     either string or string_view (or something similar)
 * @param _DelimiterType  the same type as \a _StringType or its character
 *                        type (other types may cause unpredictable results)
 */
template <typename _StringType, typename _DelimiterType>
class basic_split_view {
public:
    typedef _StringType                      string_type;
    typedef _DelimiterType                   delimiter_type;
    typedef typename string_type::value_type char_type;

    /**
     * Iterator over the split items.
     *
     * The iterator \e owns the content.
     */
    class iterator  // implements InputIterator
    {
    public:
        typedef int                               difference_type;
        typedef std::basic_string_view<char_type> value_type;
        typedef value_type*                       pointer_type;
        typedef value_type&                       reference;
        typedef std::input_iterator_tag           iterator_category;

        constexpr iterator() noexcept
            : _M_src(nullptr)
            , _M_pos(string_type::npos)
            , _M_delimiter(delimiter_type())
        {
        }
        explicit iterator(const string_type& src, delimiter_type delimiter)
            : _M_src(&src)
            , _M_pos(0)
            , _M_delimiter(delimiter)
        {
            ++*this;
        }

        reference operator*() noexcept
        {
            assert(_M_src != nullptr);
            return _M_cur;
        }
        pointer_type operator->() noexcept
        {
            assert(_M_src != nullptr);
            return &_M_cur;
        }
        iterator& operator++()
        {
            assert(_M_src != nullptr);
            if (_M_pos == string_type::npos)
            {
                _M_src = nullptr;
            }
            else
            {
                auto last_pos = _M_pos;
                _M_pos = _M_src->find(_M_delimiter, _M_pos);
                if (_M_pos != string_type::npos)
                {
                    _M_cur = std::basic_string_view<char_type>(
                        _M_src->data() + last_pos, _M_pos - last_pos);

                    // Hack: typeid(delimiter_type) == typeid(char_type)) is
                    // really wanted, but not valid as of C++17
                    if constexpr (sizeof(delimiter_type) == sizeof(char_type))
                        ++_M_pos;
                    else
                        _M_pos += _M_delimiter.size();
                }
                else
                    _M_cur = std::basic_string_view<char_type>(
                        _M_src->data() + last_pos,
                        _M_src->size() - last_pos);
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
            return _M_src == rhs._M_src &&
                   _M_pos == rhs._M_pos;
        }
        bool operator!=(const iterator& rhs) const noexcept
        {
            return !operator==(rhs);
        }

    private:
        const string_type*              _M_src;
        typename string_type::size_type _M_pos;
        value_type                      _M_cur;
        delimiter_type                  _M_delimiter;
    };

    /**
     * Constructor.
     *
     * @param src        the source input to be split
     * @param delimiter  delimiter used to split \a src; its type should be
     *                   the same as that of \a src, or its character type
     */
    constexpr explicit basic_split_view(const string_type& src,
                                        delimiter_type delimiter) noexcept
        : _M_src(src)
        , _M_delimiter(delimiter)
    {
    }

    iterator begin() const
    {
        return iterator(_M_src, _M_delimiter);
    }
    constexpr iterator end() const noexcept
    {
        return iterator();
    }

    /** Converts the view to a string vector. */
    std::vector<std::basic_string<char_type>> to_vector() const
    {
        std::vector<std::basic_string<char_type>> result;
        for (const auto& sv : *this)
            result.emplace_back(sv);
        return result;
    }
    /** Converts the view to a string_view vector. **/
    std::vector<std::basic_string_view<char_type>> to_vector_sv() const
    {
        std::vector<std::basic_string_view<char_type>> result;
        for (const auto& sv : *this)
            result.push_back(sv);
        return result;
    }

private:
    const string_type& _M_src;
    delimiter_type     _M_delimiter;
};

/**
 * Splits a string (or string_view) into lazy views.  The source input shall
 * remain unchanged when the generated basic_split_view is used in anyway.
 *
 * @param src        the source input to be split
 * @param delimiter  delimiter used to split \a src; its type should be
 *                   the same as that of \a src, or its character type
 */
template <typename _StringType, typename _DelimiterType>
constexpr basic_split_view<_StringType, _DelimiterType>
split(const _StringType& src, _DelimiterType delimiter) noexcept
{
    return basic_split_view<_StringType, _DelimiterType>(src, delimiter);
}

NVWA_NAMESPACE_END

#endif // NVWA_SPLIT_H
