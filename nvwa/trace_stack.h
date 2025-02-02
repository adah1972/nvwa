// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  trace_stack.h
 *
 * Definition of stack-like container adaptor, with the additional capability
 * of showing the last popped "stack trace".
 *
 * @date  2025-02-02
 */

#ifndef NVWA_TRACE_STACK_H
#define NVWA_TRACE_STACK_H

#include <assert.h>             // assert
#include <deque>                // std::deque
#include <utility>              // std::forward/move
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // HAVE_CXX20_RANGES

NVWA_NAMESPACE_BEGIN

template <typename Container>
class trace_stack_subrange {
public:
    using size_type = typename Container::size_type;
    using const_iterator = typename Container::const_iterator;

    trace_stack_subrange(const_iterator first, const_iterator last)
        : _M_begin(first), _M_end(last)
    {
    }

    const_iterator begin() const
    {
        return _M_begin;
    }
    const_iterator end() const
    {
        return _M_end;
    }
    const_iterator cbegin() const
    {
        return _M_begin;
    }
    const_iterator cend() const
    {
        return _M_end;
    }

    bool empty() const
    {
        return _M_begin == _M_end;
    }

    size_type size() const
    {
        return static_cast<size_type>(_M_end - _M_begin);
    }

private:
    const_iterator _M_begin;
    const_iterator _M_end;
};

template <typename T, typename Container = std::deque<T>>
class trace_stack {
public:
    using container_type = Container;
    using value_type = typename Container::value_type;
    using size_type = typename Container::size_type;
    using reference = typename Container::reference;
    using const_reference = typename Container::const_reference;

    trace_stack() = default;
    trace_stack(const Container& container) : _M_container(container) {}
    trace_stack(Container&& container) : _M_container(std::move(container))
    {
    }

    template <typename... Args>
    void emplace(Args&&... args)
    {
        discard_popped();
        _M_container.emplace_back(std::forward<Args>(args)...);
    }

    void push(const T& value)
    {
        emplace(value);
    }
    void push(T&& value)
    {
        emplace(std::move(value));
    }

    void pop()
    {
        assert(!empty());
        ++_M_trace_count;
    }

    reference top()
    {
        assert(!empty());
        return *(_M_container.end() - _M_trace_count - 1);
    }

    const_reference top() const
    {
        assert(!empty());
        return *(_M_container.end() - _M_trace_count - 1);
    }

    bool empty() const
    {
        return size() == 0;
    }

    size_type size() const
    {
        return _M_container.size() - _M_trace_count;
    }

    void discard_popped()
    {
        if (_M_trace_count == 0) {
            return;
        }
        _M_container.erase(_M_container.end() - _M_trace_count,
                           _M_container.end());
        _M_trace_count = 0;
    }

    trace_stack_subrange<Container> get_popped() const&
    {
        return {_M_container.end() - _M_trace_count, _M_container.end()};
    }

private:
    Container _M_container;
    size_type _M_trace_count{};
};

NVWA_NAMESPACE_END

#if HAVE_CXX20_RANGES
#include <ranges>

template <typename Container>
inline constexpr bool std::ranges::enable_borrowed_range<
    NVWA::trace_stack_subrange<Container>> = true;
template <typename Container>
inline constexpr bool std::ranges::enable_view<
    NVWA::trace_stack_subrange<Container>> = true;
#endif

#endif // NVWA_TRACE_STACK_H
