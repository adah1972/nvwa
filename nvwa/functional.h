// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2014 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @date  2014-11-18
 */

#ifndef NVWA_FUNCTIONAL_H
#define NVWA_FUNCTIONAL_H

#include <memory>               // std::allocator
#include <type_traits>          // std::integral_constant...
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

namespace funcimpl {

template <class T1, class T2>
struct can_reserve
{
    struct good { char dummy; };
    struct bad { char dummy[2]; };
    template <class U, void   (U::*)(size_t)> struct SFINAE1 {};
    template <class U, size_t (U::*)() const> struct SFINAE2 {};
    template <class U> static good test_reserve(SFINAE1<U, &U::reserve>*);
    template <class U> static bad  test_reserve(...);
    template <class U> static good test_size(SFINAE2<U, &U::size>*);
    template <class U> static bad  test_size(...);
    static const bool value = (sizeof(test_reserve<T1>(0)) == sizeof(good) &&
                               sizeof(test_size<T2>(0)) == sizeof(good));
};

template <class T1, class T2>
void try_reserve(T1&, const T2&, std::false_type)
{
}

template <class T1, class T2>
void try_reserve(T1& dest, const T2& src, std::true_type)
{
    dest.reserve(src.size());
}

} /* namespace funcimpl */

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
_OutCont<typename _Cont::value_type, _Alloc<typename _Cont::value_type>>
map(_Fn mapfn, const _Cont& inputs)
{
    _OutCont<typename _Cont::value_type,
        _Alloc<typename _Cont::value_type>> result;
    funcimpl::try_reserve(
        result, inputs,
        std::integral_constant<
            bool, funcimpl::can_reserve<decltype(result), _Cont>::value>());
    for (auto& item : inputs)
        result.push_back(mapfn(item));
    return result;
}

/**
 * Applies the \a reducefn function cumulatively to items in the input
 * container.
 *
 * This is similar to \c std::accumumulate, but the style is more
 * functional and more suitable for chaining operations.
 *
 * @param reducefn  the function to apply
 * @param inputs    the input container
 * @param initval   initial value for the cumulative calculation
 * @pre             \a mapfn shall take two arguments of the type of the
 *                  items in \a inputs, and the input container shall
 *                  support iteration.
 */
template <typename _Fn, class _Cont>
typename _Cont::value_type
reduce(_Fn reducefn, const _Cont& inputs,
       typename _Cont::value_type initval = typename _Cont::value_type())
{
    auto result = initval;
    for (auto& item : inputs)
        result = reducefn(result, item);
    return result;
}

/**
 * Returns the data intact to terminate the recursion.
 */
template <typename _Tp>
auto apply(const _Tp& data)
{
    return data;
}

/**
 * Applies the functions in the arguments to the data consecutively.
 *
 * @param data  the data to operate on
 * @param fn    the first function to apply
 * @param args  the rest functions to apply
 */
template <typename _Tp, typename _Fn, typename... _Fargs>
auto apply(const _Tp& data, _Fn fn, _Fargs... args)
{
    return apply(fn(data), args...);
}

/**
 * Function object to pipeline the application of function objects.
 *
 * @param _Funcs  functions to apply
 */
template <typename... _Funcs>
struct func_pipeline;

/**
 * Function object to pipeline the application of function objects.
 * This is the no-operation specialization to terminate recursion.
 */
template <>
struct func_pipeline<>
{
    template <typename _Tp>
    auto operator()(const _Tp& input)
    {
        return input;
    }
};

/**
 * Function object to pipeline the application of function objects.
 * This is the general recursive specialization to apply the first
 * function object and then call itself with the rest function objects.
 */
template <typename _First, typename... _Rest>
struct func_pipeline<_First, _Rest...>
{
    template <typename _Tp>
    auto operator()(const _Tp& input)
    {
        _First fn;
        return func_pipeline<_Rest...>()(fn(input));
    }
};

NVWA_NAMESPACE_END

#endif // NVWA_FUNCTIONAL_H
