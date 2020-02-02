// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2013-2020 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  c++_features.h
 *
 * Modern C++ feature detection macros and workarounds.
 *
 * @date  2020-02-02
 */

#ifndef NVWA_CXX_FEATURES_H
#define NVWA_CXX_FEATURES_H

#include "_nvwa.h"          // NVWA_APPLE/NVWA_APPLE_CLANG
#if NVWA_APPLE
#include <Availability.h>   // __MAC_OS_X_VERSION_MIN_REQUIRED
#endif

// Only Clang provides all these macros; they need to be defined as follows
// to get a valid expression in preprocessing when not available.
#ifndef __has_cpp_attribute
#define __has_cpp_attribute(x) 0
#endif
#ifndef __has_extension
#define __has_extension(x) 0
#endif
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef __has_include
#define __has_include(x) 0
#endif

// Detect whether C++11 mode is on (for GCC and Clang).  MSVC does not
// have a special C++11 mode, so it is always on for Visual C++ 2010 and
// later.
#if __cplusplus >= 201103L || \
    defined(__GXX_EXPERIMENTAL_CXX0X__) || \
    (defined(_MSC_VER) && _MSC_VER >= 1600)
#define NVWA_CXX11_MODE 1
#else
#define NVWA_CXX11_MODE 0
#endif

// Detect whether C++17 features can be used.
#if __cplusplus >= 201703L || \
    (defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201703)
#define NVWA_USES_CXX17 1
#else
#define NVWA_USES_CXX17 0
#endif

// Newer Clang versions cripple certain C++17 features on macOS prior to
// 10.14.
#if NVWA_USES_CXX17 && \
    ((NVWA_APPLE_CLANG && __apple_build_version__ >= 10000000) || \
     (NVWA_CLANG && __clang_major__ >= 8)) && \
    (defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && \
     __MAC_OS_X_VERSION_MIN_REQUIRED < 101400)
#define NVWA_USES_CRIPPLED_CLANG 1
#else
#define NVWA_USES_CRIPPLED_CLANG 0
#endif


/* Feature checks */

#if !defined(HAVE_CXX11_ATOMIC)
#if NVWA_CXX11_MODE && \
    ((__has_include(<atomic>) && !defined(__MINGW32__)) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (((defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405) || \
       defined(__clang__)) && \
      (!defined(__MINGW32__) || defined(_POSIX_THREADS))))
// Note: MinGW GCC, unless built with POSIX threads (as in
//       MinGW-builds), does not support atomics as of 4.8.
#define HAVE_CXX11_ATOMIC 1
#else
#define HAVE_CXX11_ATOMIC 0
#endif
#endif

#if !defined(HAVE_CXX11_AUTO_TYPE)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_auto_type) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 404))
#define HAVE_CXX11_AUTO_TYPE 1
#else
#define HAVE_CXX11_AUTO_TYPE 0
#endif
#endif

#if !defined(HAVE_CXX11_DELETED_FUNCTION)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_deleted_functions) || \
     (defined(_MSC_VER) && _MSC_VER >= 1800) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 404))
#define HAVE_CXX11_DELETED_FUNCTION 1
#else
#define HAVE_CXX11_DELETED_FUNCTION 0
#endif
#endif

#if !defined(HAVE_CXX11_EXPLICIT_CONVERSION)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_explicit_conversions) || \
     (defined(_MSC_VER) && _MSC_VER >= 1900) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405))
#define HAVE_CXX11_EXPLICIT_CONVERSION 1
#else
#define HAVE_CXX11_EXPLICIT_CONVERSION 0
#endif
#endif

#if !defined(HAVE_CXX11_FINAL)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_override_control) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 407))
#define HAVE_CXX11_FINAL 1
#else
#define HAVE_CXX11_FINAL 0
#endif
#endif

#if !defined(HAVE_CXX11_FUTURE)
#if NVWA_CXX11_MODE && \
    ((__has_include(<future>) && !defined(__MINGW32__)) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (((defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405) || \
       defined(__clang__)) && \
      (!defined(__MINGW32__) || defined(_POSIX_THREADS))))
// Note: MinGW GCC, unless built with POSIX threads (as in
//       MinGW-builds), does not support futures as of 4.8.
#define HAVE_CXX11_FUTURE 1
#else
#define HAVE_CXX11_FUTURE 0
#endif
#endif

#if !defined(HAVE_CXX11_GENERALIZED_INITIALIZER)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_generalized_initializers) || \
     (defined(_MSC_VER) && _MSC_VER >= 1800) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 404))
#define HAVE_CXX11_GENERALIZED_INITIALIZER 1
#else
#define HAVE_CXX11_GENERALIZED_INITIALIZER 0
#endif
#endif

#if !defined(HAVE_CXX11_LAMBDA)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_lambdas) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405))
#define HAVE_CXX11_LAMBDA 1
#else
#define HAVE_CXX11_LAMBDA 0
#endif
#endif

#if !defined(HAVE_CXX11_MUTEX)
#if NVWA_CXX11_MODE && \
    ((__has_include(<mutex>) && !defined(__MINGW32__)) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (((defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 403) || \
       defined(__clang__)) && \
      (!defined(__MINGW32__) || defined(_POSIX_THREADS))))
// Note: MinGW GCC, unless built with POSIX threads (as in
//       MinGW-builds), does not support std::mutex as of 4.8.
#define HAVE_CXX11_MUTEX 1
#else
#define HAVE_CXX11_MUTEX 0
#endif
#endif

#if !defined(HAVE_CXX11_NOEXCEPT)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_noexcept) || \
     (defined(_MSC_VER) && _MSC_VER >= 1900) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 406))
#define HAVE_CXX11_NOEXCEPT 1
#else
#define HAVE_CXX11_NOEXCEPT 0
#endif
#endif

#if !defined(HAVE_CXX11_NULLPTR)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_nullptr) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 406))
#define HAVE_CXX11_NULLPTR 1
#else
#define HAVE_CXX11_NULLPTR 0
#endif
#endif

#if !defined(HAVE_CXX11_OVERRIDE)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_override_control) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 407))
#define HAVE_CXX11_OVERRIDE 1
#else
#define HAVE_CXX11_OVERRIDE 0
#endif
#endif

#if !defined(HAVE_CXX11_RANGE_FOR)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_range_for) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 406))
#define HAVE_CXX11_RANGE_FOR 1
#else
#define HAVE_CXX11_RANGE_FOR 0
#endif
#endif

#if !defined(HAVE_CXX11_RVALUE_REFERENCE)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_rvalue_references) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405))
#define HAVE_CXX11_RVALUE_REFERENCE 1
#else
#define HAVE_CXX11_RVALUE_REFERENCE 0
#endif
#endif

#if !defined(HAVE_CXX11_STATIC_ASSERT)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_static_assert) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 403))
#define HAVE_CXX11_STATIC_ASSERT 1
#else
#define HAVE_CXX11_STATIC_ASSERT 0
#endif
#endif

#if !defined(HAVE_CXX11_THREAD)
#if NVWA_CXX11_MODE && \
    ((__has_include(<thread>) && !defined(__MINGW32__)) || \
     (defined(_MSC_VER) && _MSC_VER >= 1700) || \
     (((defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 404) || \
       defined(__clang__)) && \
      (!defined(__MINGW32__) || defined(_POSIX_THREADS))))
// Note: MinGW GCC, unless built with POSIX threads (as in
//       MinGW-builds), does not support std::thread as of 4.8.
#define HAVE_CXX11_THREAD 1
#else
#define HAVE_CXX11_THREAD 0
#endif
#endif

#if !defined(HAVE_CXX11_THREAD_LOCAL)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_thread_local) || \
     (defined(_MSC_VER) && _MSC_VER >= 1900) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 408))
#define HAVE_CXX11_THREAD_LOCAL 1
#else
#define HAVE_CXX11_THREAD_LOCAL 0
#endif
#endif

#if !defined(HAVE_CXX11_TYPE_TRAITS)
#if NVWA_CXX11_MODE && \
    (__has_include(<type_traits>) || \
     (defined(_MSC_VER) && _MSC_VER >= 1600) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 403))
#define HAVE_CXX11_TYPE_TRAITS 1
#else
#define HAVE_CXX11_TYPE_TRAITS 0
#endif
#endif

#if !defined(HAVE_CXX11_UNICODE_LITERAL)
#if NVWA_CXX11_MODE && \
    (__has_feature(cxx_unicode_literals) || \
     (defined(_MSC_VER) && _MSC_VER >= 1900) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 405))
#define HAVE_CXX11_UNICODE_LITERAL 1
#else
#define HAVE_CXX11_UNICODE_LITERAL 0
#endif
#endif

#if !defined(HAVE_CXX17_STRING_VIEW)
#if NVWA_USES_CXX17 && \
    (__has_include(<string_view>) || \
     (defined(_MSC_VER) && _MSC_VER >= 1910) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 700))
#define HAVE_CXX17_STRING_VIEW 1
#else
#define HAVE_CXX17_STRING_VIEW 0
#endif
#endif

#if !defined(HAVE_CXX17_ANY)
#if NVWA_USES_CXX17 && \
    ((__has_include(<any>) && !NVWA_USES_CRIPPLED_CLANG) || \
     (defined(_MSC_VER) && _MSC_VER >= 1910) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 700))
#define HAVE_CXX17_ANY 1
#else
#define HAVE_CXX17_ANY 0
#endif
#endif

#if !defined(HAVE_CXX17_OPTIONAL)
#if NVWA_USES_CXX17 && \
    ((__has_include(<optional>) && !NVWA_USES_CRIPPLED_CLANG) || \
     (defined(_MSC_VER) && _MSC_VER >= 1910) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 700))
#define HAVE_CXX17_OPTIONAL 1
#else
#define HAVE_CXX17_OPTIONAL 0
#endif
#endif

#if !defined(HAVE_CXX17_VARIANT)
#if NVWA_USES_CXX17 && \
    ((__has_include(<variant>) && !NVWA_USES_CRIPPLED_CLANG) || \
     (defined(_MSC_VER) && _MSC_VER >= 1910) || \
     (defined(__GNUC__) && __GNUC__ * 100 + __GNUC_MINOR__ >= 700))
#define HAVE_CXX17_VARIANT 1
#else
#define HAVE_CXX17_VARIANT 0
#endif
#endif


/* Workarounds */

#if HAVE_CXX11_DELETED_FUNCTION
#define _DELETED = delete
#else
#define _DELETED
#endif

#if HAVE_CXX11_FINAL
#define _FINAL final
#else
#define _FINAL
#endif

#if HAVE_CXX11_OVERRIDE
#define _OVERRIDE override
#else
#define _OVERRIDE
#endif

#if HAVE_CXX11_NOEXCEPT
#define _NOEXCEPT noexcept
#define _NOEXCEPT_(x) noexcept(x)
#else
#ifdef _MSC_VER
#define _NOEXCEPT throw ()
#else
#define _NOEXCEPT throw()
#endif
#define _NOEXCEPT_(x)
#endif

#if HAVE_CXX11_NULLPTR
#define _NULLPTR nullptr
#else
#include <stddef.h>         // NULL
#define _NULLPTR NULL
#endif

#if HAVE_CXX11_THREAD_LOCAL
#define _THREAD_LOCAL thread_local
#else
#ifdef _MSC_VER
#define _THREAD_LOCAL __declspec(thread)
#else
#define _THREAD_LOCAL __thread
#endif
#endif

#if NVWA_USES_CXX17
#define _FALLTHROUGH [[fallthrough]]
#elif defined(__clang__) && __has_cpp_attribute(clang::fallthrough)
#define _FALLTHROUGH [[clang::fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
#define _FALLTHROUGH __attribute__ ((fallthrough))
#else
#define _FALLTHROUGH
#endif

#endif // NVWA_CXX_FEATURES_H
