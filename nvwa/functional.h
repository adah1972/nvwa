// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2014-2015 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file  functional.h
 *
 * Utility templates for functional programming style.  Using this file
 * requires a C++14-compliant compiler.
 *
 * @date  2015-03-01
 */

#ifndef NVWA_FUNCTIONAL_H
#define NVWA_FUNCTIONAL_H

#include <functional>           // std::function
#include <memory>               // std::allocator
#include <type_traits>          // std::integral_constant/is_reference/...
#include <utility>              // std::declval/forward/move
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

// These are defined in C++14, but not all platforms support them.
template <bool _Bp, typename _Tp = void>
using enable_if_t = typename std::enable_if<_Bp, _Tp>::type;
template <typename _Tp>
using remove_reference_t = typename std::remove_reference<_Tp>::type;
template <typename _Tp>
using decay_t = typename std::decay<_Tp>::type;

/**
 * Returns the data intact to terminate the recursion.
 */
template <typename _Tp>
_Tp apply(_Tp&& data)
{
    return std::forward<_Tp>(data);
}

/**
 * Applies the functions in the arguments to the data consecutively.
 *
 * @param data  the data to operate on
 * @param fn    the first function to apply
 * @param args  the rest functions to apply
 */
template <typename _Tp, typename _Fn, typename... _Fargs>
decltype(auto) apply(_Tp&& data, _Fn fn, _Fargs... args)
{
    return apply(fn(std::forward<_Tp>(data)), args...);
}

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
    template <class _Up> static good __reserve(_SFINAE1<_Up, &_Up::reserve>*);
    template <class _Up> static bad  __reserve(...);
    template <class _Up> static good __size(_SFINAE2<_Up, &_Up::size>*);
    template <class _Up> static bad  __size(...);
    static const bool value =
        (sizeof(__reserve<_T1>(nullptr)) == sizeof(good) &&
         sizeof(__size<_T2>(nullptr)) == sizeof(good));
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
                             std::is_const<remove_reference_t<_Tp>>{})>
struct wrapper
{
    wrapper(_Tp&& x) : value(std::forward<_Tp>(x)) {}
    _Tp get() const { return value; }
    _Tp value;
};

// Partial specialization that copies the object used by the reference.
template <typename _Tp>
struct wrapper<_Tp, true>
{
    wrapper(_Tp&& x) : value(std::forward<_Tp>(x)) {}
    template <typename _Up = _Tp>
    enable_if_t<std::is_rvalue_reference<_Up>{}, decay_t<_Tp>> get() const
    {
        return value;
    }
    template <typename _Up = _Tp>
    enable_if_t<!std::is_rvalue_reference<_Up>{}, _Tp> get() const
    {
        return value;
    }
    decay_t<_Tp> value;
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

    static type make(const type& fn)
    {
        return fn;
    }
};

// Recursion template to curry functions with more than one parameter.
template <typename _Rs, typename _Tp, typename... _Targs>
struct curry<std::function<_Rs(_Tp, _Targs...)>>
{
    typedef typename curry<std::function<_Rs(_Targs...)>>::type remaining_type;
    typedef std::function<remaining_type(_Tp)> type;

    static type make(const std::function<_Rs(_Tp, _Targs...)>& fn)
    {
        return [fn](_Tp&& x)
        {   // Use wrapper to ensure reference types are correctly captured.
            return curry<std::function<_Rs(_Targs...)>>::make(
                [fn, w = wrapper<_Tp>(std::forward<_Tp>(x))](
                        _Targs&&... args) -> decltype(auto)
                {
                    return fn(w.get(), std::forward<_Targs>(args)...);
                });
        };
    }
};

} /* namespace detail */

/**
 * Applies the \a mapfn function to each item in the input container.
 *
 * This is similar to \c std::transform, but the style is more
 * functional and more suitable for chaining operations.
 *
 * @param mapfn   the function to apply
 * @param inputs  the input container
 * @pre           \a mapfn shall take one argument of the type of the
 *                items in \a inputs, the output container shall support
 *                \c push_back, and the input container shall support
 *                iteration.
 * @return        the container of results
 */
template <template <typename, typename> class _OutCont = std::vector,
          template <typename> class _Alloc = std::allocator,
          typename _Fn, class _Cont>
auto map(_Fn mapfn, const _Cont& inputs)
{
    typedef decltype(mapfn(std::declval<typename _Cont::value_type>()))
        result_type;
    _OutCont<result_type, _Alloc<result_type>> result;
    detail::try_reserve(
        result, inputs,
        std::integral_constant<
            bool, detail::can_reserve<decltype(result), _Cont>::value>());
    for (auto& item : inputs)
        result.push_back(mapfn(item));
    return result;
}

/**
 * Applies the \a reducefn function cumulatively to items in the input
 * container.
 *
 * This is similar to \c std::accumulate, but the style is more
 * functional and more suitable for chaining operations.
 *
 * @param reducefn  the function to apply
 * @param inputs    the input container
 * @pre             \a reducefn shall take two arguments of the type of
 *                  the items in \a inputs, and the input container
 *                  shall support iteration.
 */
template <typename _Fn, class _Cont>
auto reduce(_Fn reducefn, const _Cont& inputs)
{
    auto result = typename _Cont::value_type();
    for (auto& item : inputs)
        result = reducefn(result, item);
    return result;
}

/**
 * Applies the \a reducefn function cumulatively to a range.
 *
 * This is similar to \c std::accumulate, but the interface is
 * different, and \c reducefn is allowed to have arguments of different
 * types (the first argument shall have the same type as the function
 * return type).  Perfect forwarding allows the result to be a reference
 * type.
 *
 * @param reducefn  the function to apply
 * @param value     the first argument to be passed to \a reducefn
 * @param begin     beginning of the range
 * @param end       end of the range
 * @pre             \a reducefn shall take one argument of the result
 *                  type, and one argument of the type of the items in
 *                  \a inputs, and the input container shall support
 *                  iteration.
 */
template <typename _Rs, typename _Fn, typename _Iter>
_Rs&& reduce(_Fn reducefn, _Rs&& value, _Iter begin, _Iter end)
{
    // Recursion (instead of iteration) is used in this function, as
    // _Rs may be a reference type and a result of this type cannot
    // be assigned to (like the implementation of reduce above).
    if (begin == end)
        return value;
    _Iter current = begin;
    return reduce(reducefn, reducefn(std::forward<_Rs>(value), *current),
                  ++begin, end);
}

/**
 * Applies the \a reducefn function cumulatively to items in the input
 * container.
 *
 * This is similar to \c std::accumulate, but the style is more
 * functional and more suitable for chaining operations.  This
 * implementation allows the two arguments of \a reducefn to have
 * different types (the first argument shall have the same type as the
 * function return type).  Perfect forwarding allows the result to be a
 * reference type, and it is possible to write code like:
 *
 * @code
 * // Declaration of the output function
 * std::ostream& print(std::ostream& os, const my_type&);
 * …
 * // Dump all contents of the container_of_my_type to cout
 * nvwa::reduce(print, container_of_my_type, std::cout);
 * @endcode
 *
 * @param reducefn  the function to apply
 * @param inputs    the input container
 * @param initval   initial value for the cumulative calculation
 * @pre             \a reducefn shall take one argument of the result
 *                  type, and one argument of the type of the items in
 *                  \a inputs, and the input container shall support
 *                  iteration.
 */
template <typename _Rs, typename _Fn, class _Cont>
_Rs&& reduce(_Fn reducefn, const _Cont& inputs, _Rs&& initval)
{
    return reduce(reducefn, std::forward<_Rs>(initval),
                  std::begin(inputs), std::end(inputs));
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
 * @param fn    the function to compose
 * @return      the function object that composes the passed function
 */
template <typename _Fn>
auto compose(_Fn fn)
{
    return [fn](auto&&... x) -> decltype(auto)
    {
        return fn(std::forward<decltype(x)>(x)...);
    };
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @param fn    the last function to compose
 * @param args  the other functions to compose
 * @return      the function object that composes the passed functions
 */
template <typename _Fn, typename... _Fargs>
auto compose(_Fn fn, _Fargs... args)
{
    return [fn, args...](auto&&... x) -> decltype(auto)
    {
        return fn(compose(args...)(std::forward<decltype(x)>(x)...));
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
 * result.
 *
 * @param fn  the function to be curried as a \c std::function
 * @return    the curried function
 */
template <typename _Rs, typename... _Targs>
auto make_curry(std::function<_Rs(_Targs...)> fn)
{
    return detail::curry<std::function<_Rs(_Targs...)>>::make(fn);
}

/**
 * Makes a curried function.  The returned function takes one argument
 * at a time, and return a function that takes the next argument until
 * all arguments are exhausted, in which case it returns the final
 * result.
 *
 * @param fn  the function to be curried as a function pointer
 * @return    the curried function
 */
template <typename _Rs, typename... _Targs>
auto make_curry(_Rs(*fn)(_Targs...))
{
    return detail::curry<std::function<_Rs(_Targs...)>>::make(fn);
}

NVWA_NAMESPACE_END

#endif // NVWA_FUNCTIONAL_H
