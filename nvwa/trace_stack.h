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
 * Definition of stack-like container adaptor, with the additional
 * capability of showing the last popped "stack trace".
 *
 * @date  2025-02-08
 */

#ifndef NVWA_TRACE_STACK_H
#define NVWA_TRACE_STACK_H

#include <assert.h>             // assert
#include <deque>                // std::deque
#include <iterator>             // std::reverse_iterator
#include <utility>              // std::forward/move
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // HAVE_CXX20_RANGES

NVWA_NAMESPACE_BEGIN

/**
 * A class representing a subrange of a container, providing a range-like
 * interface.
 *
 * @param _Container  type of the container
 */
template <typename _Container>
class trace_stack_subrange {
public:
    using size_type = typename _Container::size_type;
    using const_iterator = typename _Container::const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // Constructor to initialize the subrange with iterators.
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

    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(_M_end);
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(_M_begin);
    }
    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(_M_end);
    }
    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(_M_begin);
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

/**
 * A stack-like container adaptor with additional functionalities for
 * tracing and showing the last popped items.
 *
 * @param _Tp         type of elements
 * @param _Container  type of the container (default to
 *                    \c std::deque&lt;_Tp&gt;)
 */
template <typename _Tp, typename _Container = std::deque<_Tp>>
class trace_stack {
public:
    using container_type = _Container;
    using value_type = typename container_type::value_type;
    using size_type = typename container_type::size_type;
    using reference = typename container_type::reference;
    using const_reference = typename container_type::const_reference;

    // Defaulted default constructor, copy constructor, copy assignment
    // operator, and destructor.
    trace_stack() = default;
    trace_stack(const trace_stack& rhs) = default;
    trace_stack& operator=(const trace_stack& rhs) = default;
    ~trace_stack() = default;

    // Move constructor and move assignment operator should clear the
    // trace count of the trace_stack being moved.
    trace_stack(trace_stack&& rhs) noexcept
        : _M_container(std::move(rhs._M_container)),
          _M_trace_count(rhs._M_trace_count)
    {
        rhs._M_trace_count = 0;
    }
    trace_stack& operator=(trace_stack&& rhs) noexcept
    {
        if (this != &rhs) {
            _M_container = std::move(rhs._M_container);
            _M_trace_count = rhs._M_trace_count;
            rhs._M_trace_count = 0;
        }
        return *this;
    }

    /** Constructor that takes an existing underlying container. */
    explicit trace_stack(const container_type& container)
        : _M_container(container)
    {
    }
    /** Constructor that takes a temporary underlying container. */
    explicit trace_stack(container_type&& container)
        : _M_container(std::move(container))
    {
    }

    /**
     * Constructs a new element in place at the top of the stack.  It
     * discards any previously popped elements.
     *
     * @param args  arguments to construct a new element on the stack
     */
    template <typename... _Targs>
    void emplace(_Targs&&... args)
    {
        discard_popped();
        _M_container.emplace_back(std::forward<_Targs>(args)...);
    }

    /**
     * Inserts a given element to the top of the stack.  It discards any
     * previously popped elements.
     *
     * @param value  an lvalue to add
     */
    void push(const _Tp& value)
    {
        emplace(value);
    }
    /**
     * Inserts a given element to the top of the stack.  It discards any
     * previously popped elements.
     *
     * @param value  an rvalue to add
     */
    void push(_Tp&& value)
    {
        emplace(std::move(value));
    }

    /**
     * Removes the top element from the stack.  The element is not
     * discarded, but the trace count is incremented to mark the number
     * of popped elements.
     */
    void pop()
    {
        assert(!empty());
        ++_M_trace_count;
    }

    /** Gets the top element of the stack. */
    reference top()
    {
        assert(!empty());
        return *(_M_container.end() - _M_trace_count - 1);
    }

    /** Gets the top element of the stack. */
    const_reference top() const
    {
        assert(!empty());
        return *(_M_container.end() - _M_trace_count - 1);
    }

    /** Checks whether the stack is empty. */
    bool empty() const
    {
        return size() == 0;
    }

    /** Gets the number of elements (excluding popped ones) in the stack. */
    size_type size() const
    {
        return _M_container.size() - _M_trace_count;
    }

    /**
     * Discards the elements that have been "popped" but not actually
     * removed from the container.  \c emplace() and \c push()
     * automatically call this member function.
     */
    void discard_popped()
    {
        if (_M_trace_count == 0) {
            return;
        }
        _M_container.erase(_M_container.end() - _M_trace_count,
                           _M_container.end());
        _M_trace_count = 0;
    }

    /**
     * Returns a trace_stack_subrange object representing the range of
     * recently popped items.
     *
     * @return  a trace_stack_subrange of popped items
     */
    trace_stack_subrange<container_type> get_popped() const&
    {
        return {_M_container.end() - _M_trace_count, _M_container.end()};
    }

private:
    container_type _M_container;
    size_type      _M_trace_count{};
};

NVWA_NAMESPACE_END

#if HAVE_CXX20_RANGES
#include <ranges>

// Enable borrowed_range and view for trace_stack_subrange if C++20 ranges
// are available.
template <typename _Container>
inline constexpr bool std::ranges::enable_borrowed_range<
    NVWA::trace_stack_subrange<_Container>> = true;
template <typename _Container>
inline constexpr bool std::ranges::enable_view<
    NVWA::trace_stack_subrange<_Container>> = true;
#endif

#endif // NVWA_TRACE_STACK_H
