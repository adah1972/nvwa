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
 * @date  2014-12-14
 */

#ifndef NVWA_FUNCTIONAL_H
#define NVWA_FUNCTIONAL_H

#include <functional>           // std::function
#include <memory>               // std::allocator
#include <type_traits>          // std::integral_constant...
#include <utility>              // std::forward
#include <vector>               // std::vector
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

namespace detail {

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

template <class _T1, class _T2>
void try_reserve(_T1&, const _T2&, std::false_type)
{
}

template <class _T1, class _T2>
void try_reserve(_T1& dest, const _T2& src, std::true_type)
{
    dest.reserve(src.size());
}

template <typename _Fn>
struct self_ref_func
{
    std::function<_Fn(self_ref_func)> fn;
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
_OutCont<typename _Cont::value_type, _Alloc<typename _Cont::value_type>>
map(_Fn mapfn, const _Cont& inputs)
{
    _OutCont<typename _Cont::value_type,
        _Alloc<typename _Cont::value_type>> result;
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
auto apply(_Tp&& data)
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
auto apply(_Tp&& data, _Fn fn, _Fargs... args)
{
    return apply(fn(std::forward<_Tp>(data)), args...);
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @return      the identity function
 */
template <typename _Tp>
auto compose()
{
    return apply<_Tp>;
}

/**
 * Constructs a function (object) that composes the passed functions.
 *
 * @param fn    the first function to compose (which is applied last)
 * @param args  the rest functions to compose
 * @return      the function object that composes the passed functions
 */
template <typename _Tp, typename _Fn, typename... _Fargs>
auto compose(_Fn fn, _Fargs... args)
{
    return [=](_Tp&& x)
    {
        return fn(compose<_Tp>(args...)(std::forward<_Tp>(x)));
    };
}

/**
 * Helper class to convert a function to a lambda expression suitable to
 * be passed to #fix.
 *
 * @param _Tp  type of the function argument
 * @param _Rs  type of the function return value
 */
template <typename _Tp, typename _Rs>
struct fix_function_converter
{
    typedef std::function<_Rs(_Tp)>                fn_1st_ord;
    typedef std::function<fn_1st_ord(fn_1st_ord)>  fn_2nd_ord;

    fn_2nd_ord
    operator()(std::function<_Rs(std::function<_Rs(_Tp)>, _Tp)> fn) const
    {
        return fn_2nd_ord([fn](fn_1st_ord f1)
                          {
                              return fn_1st_ord(
                                  [fn, f1](_Tp&& x)
                                  {
                                      return fn(f1, std::forward<_Tp>(x));
                                  });
                          });
    }
};

/**
 * The simple fixed-point combinator using lazy evaluation of definition.
 *
 * @param _Tp  type of the function argument
 * @param _Rs  type of the function return value
 */
template <typename _Tp, typename _Rs>
struct fix_simple
{
    typedef std::function<_Rs(_Tp)>                fn_1st_ord;
    typedef std::function<fn_1st_ord(fn_1st_ord)>  fn_2nd_ord;

    fn_1st_ord operator()(fn_2nd_ord f2) const
    {   // Y f = f (Y f)
        fn_1st_ord y_f = [this, f2](_Tp&& x)
        {   // λx.Y (f x)
            return operator()(f2)(std::forward<_Tp>(x));
        };
        return f2(y_f);
    }
};

/**
 * The classic Curry-style fixed-point combinator.
 *
 * @param _Tp  type of the function argument
 * @param _Rs  type of the function return value
 */
template <typename _Tp, typename _Rs>
struct fix_curry
{
    typedef std::function<_Rs(_Tp)>                fn_1st_ord;
    typedef std::function<fn_1st_ord(fn_1st_ord)>  fn_2nd_ord;
    typedef detail::self_ref_func<fn_1st_ord>      fn_self_ref;

    fn_1st_ord operator()(fn_2nd_ord f2) const
    {   // Y = λf.(λx.x x) (λx.f (x x))
        fn_self_ref r = {
            [f2](fn_self_ref s)
            {   // λs.f(λx.(s s) x)
                return f2(fn_1st_ord(
                            [s](_Tp&& x)
                            {
                                return s.fn(s)(std::forward<_Tp>(x));
                            }));
            }
        };
        return r.fn(r);
    }
};

/**
 * Helper function to generate the fixed point without specifying the
 * types.
 *
 * @param f2  the second-order function to combine with
 * @return    the fixed point
 */
template <template <typename, typename> class _Fixer,
          typename _Tp, typename _Rs>
std::function<_Rs(_Tp)> make_fix(
    std::function<std::function<_Rs(_Tp)>(std::function<_Rs(_Tp)>)> f2)
{
    return _Fixer<_Tp, _Rs>()(f2);
}

/**
 * Generates the fixed point using the simple fixed-point combinator.
 *
 * @param f2  the second-order function to combine with
 * @return    the fixed point
 */
template <typename _Tp, typename _Rs>
std::function<_Rs(_Tp)> make_fix_simple(
    std::function<std::function<_Rs(_Tp)>(std::function<_Rs(_Tp)>)> f2)
{
    return make_fix<fix_simple>(f2);
}

/**
 * Generates the fixed point using the Curry-style fixed-point combinator.
 *
 * @param f2  the second-order function to combine with
 * @return    the fixed point
 */
template <typename _Tp, typename _Rs>
std::function<_Rs(_Tp)> make_fix_curry(
    std::function<std::function<_Rs(_Tp)>(std::function<_Rs(_Tp)>)> f2)
{
    return make_fix<fix_curry>(f2);
}

NVWA_NAMESPACE_END

#endif // NVWA_FUNCTIONAL_H
