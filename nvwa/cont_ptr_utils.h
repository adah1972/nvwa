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
 * @file    cont_ptr_utils.h
 *
 * Utility functors for containers of pointers (adapted from Scott
 * Meyers' <em>Effective STL</em>).
 *
 * @version 1.6, 2010/05/22
 * @author  Wu Yongwei
 *
 */

#ifndef _CONT_PTR_UTILS_H
#define _CONT_PTR_UTILS_H

/**
 * Functor to return objects pointed by a container of pointers.
 *
 * A typical usage might be like:
 * @code
 * vector<Object*> v;
 * ...
 * transform(v.begin(), v.end(),
 *           ostream_iterator<Object>(cout, " "),
 *           dereference());
 * @endcode
 */
struct dereference
{
    template <typename _Tp>
    const _Tp& operator()(const _Tp* pointer) const
    {
        return *pointer;
    }
};

/**
 * Functor to compare objects pointed by a container of pointers.
 *
 * @code
 * vector<Object*> v;
 * ...
 * sort(v.begin(), v.end(), dereference_less());
 * @endcode
 * or
 * @code
 * set<Object*, dereference_less> s;
 * @endcode
 */
struct dereference_less
{
    template <typename _Pointer>
    bool operator()(const _Pointer& pointer1, const _Pointer& pointer2) const
    {
        return *pointer1 < *pointer2;
    }
};

/**
 * Functor to delete objects pointed by a container of pointers.
 *
 * A typical usage might be like:
 * @code
 * list<Object*> l;
 * ...
 * for_each(l.begin(), l.end(), delete_object());
 * @endcode
 */
struct delete_object
{
    template <typename _Tp>
    void operator()(_Tp* pointer) const
    {
        delete pointer;
    }
};

/**
 * Functor to output objects pointed by a container of pointers.
 *
 * A typical usage might be like:
 * @code
 * list<Object*> l;
 * ...
 * for_each(l.begin(), l.end(), output_object<ostream>(cout, " "));
 * @endcode
 */
template <typename _OutputStrm, typename _StringType = const char*>
struct output_object
{
    output_object(_OutputStrm& outs, const _StringType& sep)
        : _M_outs(outs), _M_sep(sep)
    {}

    template <typename _Pointer>
    void operator()(const _Pointer& pointer) const
    {
        _M_outs << *pointer << _M_sep;
    }

private:
    _OutputStrm& _M_outs;
    _StringType  _M_sep;
};

#endif // _CONT_PTR_UTILS_H
