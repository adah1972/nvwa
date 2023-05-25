// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2022 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  static_assert.h
 *
 * Template class to check validity duing compile time (adapted from Loki).
 *
 * @date  2022-04-04
 */

#ifndef STATIC_ASSERT

#include "c++_features.h"   // HAVE_CXX11_STATIC_ASSERT
#include "_nvwa.h"          // NVWA_NAMESPACE_*

#if HAVE_CXX11_STATIC_ASSERT

#define STATIC_ASSERT(_Expr, _Msg) static_assert(_Expr, #_Msg)

#else

NVWA_NAMESPACE_BEGIN

template <bool> struct compile_time_error;
template <>     struct compile_time_error<true> {};

#define STATIC_ASSERT(_Expr, _Msg) \
    { \
        NVWA::compile_time_error<((_Expr) != 0)> ERROR_##_Msg; \
        (void)ERROR_##_Msg; \
    }

NVWA_NAMESPACE_END

#endif // HAVE_CXX11_STATIC_ASSERT

NVWA_NAMESPACE_BEGIN

template <typename T>
struct always_false {
    static const bool value = false;
};

NVWA_NAMESPACE_END

#endif // STATIC_ASSERT
