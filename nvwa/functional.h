// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2014-2018 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  functional.h
 *
 * Utility templates for functional programming style.  Using this file
 * requires a C++14-compliant compiler.
 *
 * @date  2018-01-02
 */

#ifndef NVWA_FUNCTIONAL_H
#define NVWA_FUNCTIONAL_H

#include <cassert>              // assert
#include <functional>           // std::function
#include <iterator>             // std::begin/iterator_traits
#include <memory>               // std::allocator
#include <stdexcept>            // std::logic_error
#include <string>               // std::string
#include <tuple>                // std::tuple
#include <type_traits>          // std::decay_t/is_const/integral_constant/...
#include <utility>              // std::declval/forward/move/index_sequence
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

namespace detail {

// Struct to check whether _T1 has a suitable reserve member function
// and _T2 has a suitable size member function.
template <class _T1, class _T2>
struct can_reserve
{
    struct good { char dummy; };
    struct bad { char dummy[2]; };
    template <class _Up, void   (_Up::*)(size_t)> struct _SFINAE1 {};
    template <class _Up, size_t (_Up::*)() const> struct _SFINAE2 {};
    template <class _Up> static good reserve(_SFINAE1<_Up, &_Up::reserve>*);
    template <class _Up> static bad  reserve(...);
    template <class _Up> static good size(_SFINAE2<_Up, &_Up::size>*);
    template <class _Up> static bad  size(...);
    static const bool value =
        (sizeof(reserve<_T1>(nullptr)) == sizeof(good) &&
         sizeof(size<_T2>(nullptr)) == sizeof(good));
};

// Does nothing since can_reserve::value is false.
template <class _T1, class _T2>
void try_reserve(_T1&, const _T2&, std::false_type)
{
}

// Reserves the destination with the source's size.
template <class _T1, class _T2>
void try_reserve(_T1& dest, const _T2& src, std::true_type)
{
    dest.reserve(src.size());
}

// Applies the function to indexed elements in the tuple, and combines
// the result as a tuple.
template <typename _Fn, class _Tuple, std::size_t... _I>
constexpr auto tuple_fmap_impl(_Fn&& f, _Tuple&& t,
                               std::index_sequence<_I...>)
{
    return std::make_tuple(f(std::get<_I>(std::forward<_Tuple>(t)))...);
}

// Applies the function to the given value and the indexed element of
// the tuple.  It stops the recursion.
template <typename _Rs, typename _Fn, class _Tuple>
constexpr _Rs tuple_reduce_impl(_Fn&&, _Rs&& value, _Tuple&&,
                                std::index_sequence<>)
{
    return std::forward<_Rs>(value);
}

// Recursively applies the function to the given value and indexed
// elements of the tuple.
template <typename _Rs, typename _Fn, class _Tuple, std::size_t _I,
          std::size_t... _J>
constexpr _Rs tuple_reduce_impl(_Fn&& f, _Rs&& value, _Tuple&& t,
                                std::index_sequence<_I, _J...>)
{
    return tuple_reduce_impl(std::forward<_Fn>(f),
                             f(std::forward<_Rs>(value), std::get<_I>(t)),
                             std::forward<_Tuple>(t),
                             std::index_sequence<_J...>());
}

// Applies the function with the indexed elements of the tuple as
// arguments.
template <typename _Fn, class _Tuple, std::size_t... _I>
constexpr decltype(auto) tuple_apply_impl(_Fn&& f, _Tuple&& t,
                                          std::index_sequence<_I...>)
{
    return f(std::get<_I>(std::forward<_Tuple>(t))...);
}

// Struct to wrap a function for self-reference.
template <typename _Fn>
struct self_ref_func
{
    std::function<_Fn(self_ref_func)> fn;
};

// Struct to wrap data, which may or may not be a reference.
template <typename _Tp,
          bool _Deep_copy = std::is_rvalue_reference<_Tp>{} ||
                            (std::is_lvalue_reference<_Tp>{} &&
                             std::is_const<std::remove_reference_t<_Tp>>{})>
struct safe_wrapper
{
    safe_wrapper(_Tp&& x) : value(std::forward<_Tp>(x)) {}
    _Tp get() const { return value; }
    _Tp value;
};

// Partial specialization that copies the object used by the reference.
template <typename _Tp>
struct safe_wrapper<_Tp, true>
{
    safe_wrapper(_Tp&& x) : value(std::forward<_Tp>(x)) {}
    template <typename _Up = _Tp>
    std::enable_if_t<std::is_rvalue_reference<_Up>{}, std::decay_t<_Tp>>
    get() const
    {
        return value;
    }
    template <typename _Up = _Tp>
    std::enable_if_t<!std::is_rvalue_reference<_Up>{}, _Tp>
    get() const
    {
        return value;
    }
    std::decay_t<_Tp> value;
};

// Declaration of curry, to be specialized below.  The code is based on
// Julian Becker's StackOverflow answer at <url:http://tinyurl.com/cxx-curry>.
template <typename _Fn>
struct curry;

// Termination of currying, which returns the original function.
template <typename _Rs, typename _Tp>
struct curry<std::function<_Rs(_Tp)>>
{
    typedef std::function<_Rs(_Tp)> type;

    static type make(type f)
    {
        return f;
    }
};

// Recursion template to curry functions with more than one parameter.
template <typename _Rs, typename _Tp, typename... _Targs>
struct curry<std::function<_Rs(_Tp, _Targs...)>>
{
    typedef typename curry<std::function<_Rs(_Targs...)>>::type remaining_type;
    typedef std::function<remaining_type(_Tp)> type;

    static type make(std::function<_Rs(_Tp, _Targs...)> f)
    {
        return [f](_Tp&& x)
        {   // Use wrapper to ensure reference types are correctly captured.
            return curry<std::function<_Rs(_Targs...)>>::make(
                [f, w = safe_wrapper<_Tp>(std::forward<_Tp>(x))](
                        _Targs&&... args) -> decltype(auto)
                {
                    return f(w.get(), std::forward<_Targs>(args)...);
                });
        };
    }
};

using std::begin;
using std::end;

// Function declarations to resolve begin/end using argument-dependent lookup
template <class _Rng>
auto adl_begin(_Rng&& rng) -> decltype(begin(rng));
template <class _Rng>
auto adl_end(_Rng&& rng) -> decltype(end(rng));

// Type alias to get the value type of a container or range
template <class _Rng>
using value_type = typename std::iterator_traits<decltype(
    adl_begin(std::declval<_Rng>()))>::value_type;

using std::swap;

// Function declaration to resolve swap using argument-dependent lookup
template <typename _Tp>
void adl_swap(_Tp& lhs, _Tp& rhs) noexcept(noexcept(swap(lhs, rhs)));

} /* namespace detail */

/** Class for bad optional access exception. */
class bad_optional_access : public std::logic_error
{
public:
    bad_optional_access()
        : std::logic_error("optional has no valid value now") {}
};

template <typename _Tp, bool = std::is_trivially_destructible<_Tp>::value>
class optional_base;

template <typename _Tp>
class optional_base<_Tp, false>
{
public:
    typedef _Tp value_type;

    constexpr optional_base() : _M_dummy(), _M_engaged(false) {}
    constexpr explicit optional_base(const _Tp& x)
        : _M_value(x)
        , _M_engaged(true)
    {}
    constexpr explicit optional_base(_Tp&& x) noexcept
        : _M_value(std::move(x))
        , _M_engaged(true)
    {}
    constexpr optional_base(const optional_base& rhs)
        : _M_engaged(rhs._M_engaged)
    {
        if (rhs._M_engaged)
            new(&_M_value) _Tp(rhs._M_value);
    }
    constexpr optional_base(optional_base&& rhs) noexcept
        : _M_engaged(rhs._M_engaged)
    {
        if (rhs._M_engaged)
        {
            new(&_M_value) _Tp(std::move(rhs._M_value));
            rhs.reset();
        }
    }
    ~optional_base()
    {
        if (_M_engaged)
            _M_value.~_Tp();
    }
    void reset() noexcept
    {
        if (_M_engaged)
        {
            _M_value.~_Tp();
            _M_engaged = false;
        }
    }

protected:
    union
    {
        char       _M_dummy;
        value_type _M_value;
    };
    bool _M_engaged;
};

template <typename _Tp>
class optional_base<_Tp, true>
{
public:
    typedef _Tp value_type;

    constexpr optional_base() : _M_dummy(), _M_engaged(false) {}
    constexpr explicit optional_base(const _Tp& x)
        : _M_value(x)
        , _M_engaged(true)
    {}
    constexpr explicit optional_base(_Tp&& x) noexcept
        : _M_value(std::move(x))
        , _M_engaged(true)
    {}
    constexpr optional_base(const optional_base& rhs)
        : _M_engaged(rhs._M_engaged)
    {
        if (rhs._M_engaged)
            new(&_M_value) _Tp(rhs._M_value);
    }
    constexpr optional_base(optional_base&& rhs) noexcept
        : _M_engaged(rhs._M_engaged)
    {
        if (rhs._M_engaged)
        {
            new(&_M_value) _Tp(std::move(rhs._M_value));
            rhs.reset();
        }
    }
    void reset() noexcept
    {
        _M_engaged = false;
    }

protected:
    union
    {
        char _M_dummy;
        value_type _M_value;
    };
    bool _M_engaged;
};

/**
 * Class for optional values.  It was initially modelled after the Maybe
 * type in Haskell, but the interface was later changed to be more like
 * (but not fully conformant to) the C++17 std::optional.
 *
 * @param _Tp  the optional type to store
 */
template <typename _Tp>
class optional : public optional_base<_Tp>
{
    static_assert(std::is_nothrow_destructible<_Tp>::value,
                  "optional type must be nothrow destructible");

    using optional_base<_Tp>::_M_value;
    using optional_base<_Tp>::_M_engaged;

public:
    using optional_base<_Tp>::reset;

    constexpr optional() noexcept {}
    constexpr optional(const optional& rhs) : optional_base<_Tp>(rhs) {}
    constexpr optional(optional&& rhs) noexcept(
        std::is_nothrow_move_constructible<_Tp>::value)
        : optional_base<_Tp>(std::move(rhs))
    {}
    constexpr optional(const _Tp& x) : optional_base<_Tp>(x) {}
    constexpr optional(_Tp&& x) noexcept(
        std::is_nothrow_move_constructible<_Tp>::value)
        : optional_base<_Tp>(std::move(x))
    {}
    optional& operator=(const optional& rhs)
    {
        using std::swap;
        optional temp(rhs);
        swap(*this, temp);
        return *this;
    }
    optional& operator=(optional&& rhs) noexcept(
        std::is_nothrow_move_assignable<_Tp>::value &&
        std::is_nothrow_move_constructible<_Tp>::value)
    {
        using std::swap;
        optional temp(std::move(rhs));
        swap(*this, temp);
        return *this;
    }

    constexpr _Tp* operator->()
    {
        assert(_M_engaged);
        return &_M_value;
    }
    constexpr const _Tp* operator->() const
    {
        assert(_M_engaged);
        return &_M_value;
    }
    constexpr _Tp& operator*() &
    {
        assert(_M_engaged);
        return _M_value;
    }
    constexpr const _Tp& operator*() const&
    {
        assert(_M_engaged);
        return _M_value;
    }
    constexpr _Tp&& operator*() &&
    {
        assert(_M_engaged);
        return std::move(_M_value);
    }

    constexpr bool has_value() const noexcept
    {
        return _M_engaged;
    }

    constexpr _Tp& value() &
    {
        if (_M_engaged)
            return _M_value;
        else
            throw bad_optional_access();
    }
    constexpr const _Tp& value() const&
    {
        if (_M_engaged)
            return _M_value;
        else
            throw bad_optional_access();
    }
    constexpr _Tp&& value() &&
    {
        if (_M_engaged)
            return std::move(_M_value);
        else
            throw bad_optional_access();
    }

    template <typename _Up>
    constexpr _Tp value_or(_Up&& default_value) const&
    {
        if (_M_engaged)
            return _M_value;
        else
            return default_value;
    }
    template <typename _Up>
    constexpr _Tp value_or(_Up&& default_value) &&
    {
        if (_M_engaged)
            return std::move(_M_value);
        else
            return default_value;
    }

    void swap(optional& rhs) noexcept(
        std::is_nothrow_move_constructible<_Tp>::value &&
        noexcept(detail::adl_swap(std::declval<_Tp&>(),
                                  std::declval<_Tp&>())))
    {
        using std::swap;
        if (has_value())
        {
            if (rhs.has_value())
                swap(_M_value, rhs._M_value);
            else
                new(&rhs) optional(std::move(*this));
        }
        else
        {
            if (rhs.has_value())
                new(this) optional(std::move(rhs));
        }
    }
    template <typename... _Targs>
    void emplace(_Targs&&... args)
    {
        reset();
        new (&_M_value) _Tp(std::forward<decltype(args)>(args)...);
        _M_engaged = true;
    }
};

template <typename _Tp>
constexpr optional<std::decay_t<_Tp>> make_optional(_Tp&& x)
{
    return optional<std::decay_t<_Tp>>(std::forward<_Tp>(x));
}

template <typename _Tp>
constexpr bool has_value(const optional<_Tp>& x) noexcept
{
    return x.has_value();
}

template <typename _Tp, typename... _Targs>
constexpr bool has_value(const optional<_Tp>& first,
                         const optional<_Targs>&... other) noexcept
{
    return first.has_value() && has_value(other...);
}

template <typename _Tp>
void swap(optional<_Tp>& lhs,
          optional<_Tp>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

/**
 * Lifts a function so that it takes optionals and returns an optional.
 *
 * @param f  function to lift
 * @return   the lifted function
 */
template <typename _Fn>
auto lift_optional(_Fn&& f)
{
    return [f = std::forward<_Fn>(f)](auto&&... args)
    {
        typedef std::decay_t<decltype(
            f(std::forward<decltype(args)>(args).value()...))>
            result_type;
        if (has_value(args...))
            return optional<result_type>(
                f(std::forward<decltype(args)>(args).value()...));
        else
            return optional<result_type>();
    };
}

/**
 * Applies a function to the values of optionals if they are all valid.
 *
 * If any of the optionals are invalid, the result is invalid too.
 *
 * @param f     the function to apply
 * @param args  optionals whose values will be passed to \a f
 * @return      an optional that either is invalid (if any of the input
 *              is invalid) or contains the output of \a f
 */
template <typename _Fn, typename... _Opt>
constexpr auto apply(_Fn&& f, _Opt&&... args) -> decltype(
    has_value(args...),
    optional<
        std::decay_t<decltype(f(std::forward<_Opt>(args).value()...))>>())
{
    typedef std::decay_t<decltype(f(std::forward<_Opt>(args).value()...))>
        result_type;
    if (has_value(args...))
        return optional<result_type>(
            f(std::forward<_Opt>(args).value()...));
    else
        return optional<result_type>();
}

/**
 * Applies the function with all elements of the tuple as arguments.  It
 * is exactly like the C++17 std::apply.
 *
 * @param f  the function to apply
 * @param t  the tuple that can expand to the function arguments
 * @return   the result of the function applied
 */
template <typename _Fn, class _Tuple>
constexpr auto apply(_Fn&& f, _Tuple&& t)
    -> decltype(detail::tuple_apply_impl(
        std::forward<_Fn>(f),
        std::forward<_Tuple>(t),
        std::make_index_sequence<
            std::tuple_size<std::decay_t<_Tuple>>::value>()))
{
    return detail::tuple_apply_impl(
        std::forward<_Fn>(f),
        std::forward<_Tuple>(t),
        std::make_index_sequence<
            std::tuple_size<std::decay_t<_Tuple>>::value>());
}

/**
 * Applies a function to both elements of a pair, and makes the results
 * a pair.
 *
 * @param f     the function to apply
 * @param args  pair of arguments to pass
 * @return      pair of results of function invocation
 */
template <typename _Fn, typename _T1, typename _T2>
constexpr auto fmap(_Fn&& f, const std::pair<_T1, _T2>& args)
{
    return std::make_pair(f(args.first), f(args.second));
}

/**
 * Applies a function to all elements of a tuple, and makes the results
 * a tuple.
 *
 * @param f     the function to apply
 * @param args  tuple of arguments to pass
 * @return      tuple of results of function invocation
 */
template <typename _Fn, typename... _Targs>
constexpr auto fmap(_Fn&& f, const std::tuple<_Targs...>& args)
{
    return detail::tuple_fmap_impl(std::forward<_Fn>(f), args,
                                   std::index_sequence_for<_Targs...>());
}

/**
 * Applies a function to each element in the input range.
 *
 * This is similar to \c std::transform, but the style is more
 * functional and more suitable for chaining operations.
 *
 * @param f       the function to apply
 * @param inputs  the input range
 * @pre           \a f shall take one argument of the type of the
 *                elements in \a inputs, the output container shall
 *                support \c push_back, and the input range shall
 *                support iteration.
 * @return        the container of results
 */
template <template <typename, typename> class _OutCont = std::vector,
          template <typename> class _Alloc = std::allocator,
          typename _Fn, class _Rng>
constexpr auto fmap(_Fn&& f, _Rng&& inputs) -> decltype(
    detail::adl_begin(inputs), detail::adl_end(inputs),
    _OutCont<
        std::decay_t<decltype(f(*detail::adl_begin(inputs)))>,
        _Alloc<std::decay_t<decltype(f(*detail::adl_begin(inputs)))>>>())
{
    typedef std::decay_t<decltype(f(*detail::adl_begin(inputs)))>
        result_type;
    _OutCont<result_type, _Alloc<result_type>> result;
    detail::try_reserve(
        result, inputs,
        std::integral_constant<
            bool, detail::can_reserve<decltype(result), _Rng>::value>());
    for (auto& item : inputs)
        result.push_back(f(item));
    return result;
}

/**
 * Applies a function cumulatively to all elements of a tuple.
 *
 * @param f      the function to apply
 * @param value  the first argument to be passed to the function
 * @param args   the input tuple
 * @pre          \a f shall take one argument of the result type, and
 *               one argument of the type of the elements in \a args.
 */
template <typename _Rs, typename _Fn, typename... _Targs>
constexpr auto reduce(_Fn&& f,
                      const std::tuple<_Targs...>& args,
                      _Rs&& value)
{
    return detail::tuple_reduce_impl(std::forward<_Fn>(f),
                                     std::forward<_Rs>(value),
                                     args,
                                     std::index_sequence_for<_Targs...>());
}

/**
 * Applies a function cumulatively to elements in the input range.
 *
 * This is similar to \c std::accumulate, but the style is more
 * functional and more suitable for chaining operations.
 *
 * @param f       the function to apply
 * @param inputs  the input range
 * @pre           \a f shall take two arguments of the type of the
 *                elements in \a inputs, and the input range shall
 *                support iteration.
 */
template <typename _Fn, class _Rng>
constexpr auto reduce(_Fn&& f, _Rng&& inputs)
{
    auto result = typename detail::value_type<_Rng>();
    for (auto& item : inputs)
        result = f(result, item);
    return result;
}

/**
 * Applies a function cumulatively to a range.
 *
 * This is similar to \c std::accumulate, but the interface is
 * different, and \a f is allowed to have arguments of different types
 * (the first argument shall have the same type as the function return
 * type).  Perfect forwarding allows the result to be a reference type.
 *
 * @param f      the function to apply
 * @param value  the first argument to be passed to \a f
 * @param begin  beginning of the range
 * @param end    end of the range
 * @pre          \a f shall take one argument of the result type, and
 *               one argument of the type of the elements in \a inputs,
 *               and the input range shall support iteration.
 */
template <typename _Rs, typename _Fn, typename _Iter>
constexpr _Rs&& reduce(_Fn&& f, _Rs&& value, _Iter begin, _Iter end)
{
    // Recursion (instead of iteration) is used in this function, as
    // _Rs may be a reference type and a result of this type cannot
    // be assigned to (like the implementation of reduce above).
    if (begin == end)
        return std::forward<_Rs>(value);
    _Iter current = begin;
    decltype(auto) reduced_once = f(std::forward<_Rs>(value), *current);
    return reduce(std::forward<_Fn>(f), reduced_once, ++begin, end);
}

/**
 * Applies a function cumulatively to elements in the input range.
 *
 * This is similar to \c std::accumulate, but the style is more
 * functional and more suitable for chaining operations.  This
 * implementation allows the two arguments of \a f to have different
 * types (the first argument shall have the same type as the function
 * return type).  Perfect forwarding allows the result to be a reference
 * type, and it is possible to write code like:
 *
 * @code
 * // Declaration of the output function
 * std::ostream& print(std::ostream& os, const my_type&);
 * …
 * // Dump all contents of the container_of_my_type to cout
 * nvwa::reduce(print, container_of_my_type, std::cout);
 * @endcode
 *
 * @param f        the function to apply
 * @param inputs   the input range
 * @param initval  initial value for the cumulative calculation @pre
 *                 \a f shall take one argument of the result type, and
 *                 one argument of the type of the elements in \a inputs,
 *                 and the input range shall support iteration.
 */
template <typename _Rs, typename _Fn, class _Rng>
constexpr auto reduce(_Fn&& f, _Rng&& inputs, _Rs&& initval)
    -> decltype(f(initval, *detail::adl_begin(inputs)))
{
    using std::begin;
    using std::end;
    return reduce(std::forward<_Fn>(f), std::forward<_Rs>(initval),
                  begin(inputs), end(inputs));
}

/**
 * Makes a two-argument function accept a pair instead.
 *
 * @param f  a function that accepts two arguments
 * @return   a function that accepts a pair
 */
template <typename _T1, typename _T2, typename _Fn>
constexpr auto wrap_args_as_pair(_Fn&& f)
{
    return [f = std::forward<_Fn>(f)](const std::pair<_T1, _T2>& arg)
        -> decltype(auto)
    {
        return f(arg.first, arg.second);
    };
}

/**
 * Makes a function accept a tuple as its arguments.  The tuple shall
 * contain exactly the same type/number of argument as the function
 * needs.
 *
 * @param f  a function that accepts two arguments
 * @return   a function that accepts a pair
 */
template <typename _Tuple, typename _Fn>
constexpr auto wrap_args_as_tuple(_Fn&& f)
{
    return [f = std::forward<_Fn>(f)](_Tuple&& t) -> decltype(auto)
    {
        return nvwa::apply(f, std::forward<_Tuple>(t));
    };
}

/**
 * Returns the data intact to terminate the recursion.
 */
template <typename _Tp>
constexpr _Tp pipeline(_Tp&& data)
{
    return std::forward<_Tp>(data);
}

/**
 * Applies the functions in the arguments to the data consecutively.
 *
 * @param data  the data to operate on
 * @param f     the first function to apply
 * @param args  the rest functions to apply
 */
template <typename _Tp, typename _Fn, typename... _Fargs>
constexpr decltype(auto) pipeline(_Tp&& data, _Fn&& f, _Fargs&&... args)
{
    return pipeline(f(std::forward<_Tp>(data)),
                    std::forward<_Fargs>(args)...);
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @return      the forwarding function
 */
auto compose()
{
    return [](auto&& x) -> decltype(auto)
    {
        return std::forward<decltype(x)>(x);
    };
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @param f  the function to compose
 * @return   the function object that composes the passed function
 */
template <typename _Fn>
auto compose(_Fn f)
{
    return [f](auto&&... x) -> decltype(auto)
    {
        return f(std::forward<decltype(x)>(x)...);
    };
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @param f     the last function to compose
 * @param args  the other functions to compose
 * @return      the function object that composes the passed functions
 */
template <typename _Fn, typename... _Fargs>
auto compose(_Fn f, _Fargs... args)
{
    return [f, args...](auto&&... x) -> decltype(auto)
    {
        return f(compose(args...)(std::forward<decltype(x)>(x)...));
    };
}

/**
 * Generates the fixed point using the simple fixed-point combinator.
 * This function takes a non-curried function of two parameters.
 *
 * @param f  the second-order function to combine with
 * @return   the fixed point
 */
template <typename _Rs, typename _Tp>
std::function<_Rs(_Tp)> fix_simple(
    std::function<_Rs(std::function<_Rs(_Tp)>, _Tp)> f)
{   // Y f = f (λx.(Y f) x)
    return [f](_Tp&& x)
    {
        return f([f](_Tp&& x)
                 {
                     return fix_simple(f)(std::forward<_Tp>(x));
                 },
                 std::forward<_Tp>(x));
    };
}

/**
 * Generates the fixed point using the simple fixed-point combinator.
 * This function takes a curried function of one parameter.
 *
 * @param f  the second-order function to combine with
 * @return   the fixed point
 */
template <typename _Rs, typename _Tp>
std::function<_Rs(_Tp)> fix_simple(
    std::function<std::function<_Rs(_Tp)>(std::function<_Rs(_Tp)>)> f)
{   // Y f = f (λx.(Y f) x)
    return f([f](_Tp&& x)
             {
                 return fix_simple(f)(std::forward<_Tp>(x));
             });
}

/**
 * Generates the fixed point using the Curry-style fixed-point combinator.
 * This function takes a curried function of one parameter.
 *
 * @param f  the second-order function to combine with
 * @return   the fixed point
 */
template <typename _Rs, typename _Tp>
std::function<_Rs(_Tp)> fix_curry(
    std::function<std::function<_Rs(_Tp)>(std::function<_Rs(_Tp)>)> f)
{   // Y = λf.(λx.x x) (λx.f (λy.(x x) y))
    typedef std::function<_Rs(_Tp)>            fn_1st_ord;
    typedef detail::self_ref_func<fn_1st_ord>  fn_self_ref;

    fn_self_ref r = {
        [f](fn_self_ref x)
        {   // λx.f (λy.(x x) y)
            return f(fn_1st_ord([x](_Tp&& y)
                                {
                                    return x.fn(x)(std::forward<_Tp>(y));
                                }));
        }
    };
    return r.fn(r);
}

/**
 * Makes a curried function.  The returned function takes one argument
 * at a time, and return a function that takes the next argument until
 * all arguments are exhausted, in which case it returns the final
 * result.  This overload takes an std::function and can deduce its
 * argument types and return type.
 *
 * @param f  the function to be curried as a \c std::function
 * @return   the curried function
 */
template <typename _Rs, typename... _Targs>
auto make_curry(std::function<_Rs(_Targs...)> f)
{
    return detail::curry<std::function<_Rs(_Targs...)>>::make(std::move(f));
}

/**
 * Makes a curried function.  The returned function takes one argument
 * at a time, and return a function that takes the next argument until
 * all arguments are exhausted, in which case it returns the final
 * result.  This overload takes a pointer to function and can deduce its
 * argument types and return type.
 *
 * @param f  the function to be curried as a function pointer
 * @return   the curried function
 */
template <typename _Rs, typename... _Targs>
auto make_curry(_Rs(*f)(_Targs...))
{
    return detail::curry<std::function<_Rs(_Targs...)>>::make(f);
}

/**
 * Makes a curried function.  The returned function takes one argument
 * at a time, and return a function that takes the next argument until
 * all arguments are exhausted, in which case it returns the final
 * result.  This overload takes a generic function object, and the
 * function type must be specified when this function template is
 * instantiated.
 *
 * @param f  the function to be curried as a function pointer
 * @return   the curried function
 */
template <typename _FnType, typename _Fn>
auto make_curry(_Fn&& f)
{
    return detail::curry<std::function<_FnType>>::make(
        std::forward<_Fn>(f));
}

NVWA_NAMESPACE_END

#endif // NVWA_FUNCTIONAL_H
