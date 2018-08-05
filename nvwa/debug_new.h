// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2017 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  debug_new.h
 *
 * Header file for checking leaks caused by unmatched new/delete.
 *
 * @date  2017-09-01
 */

#ifndef NVWA_DEBUG_NEW_H
#define NVWA_DEBUG_NEW_H

#include <new>                  // size_t/std::bad_alloc
#include <cstdio>               // FILE
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NOEXCEPT

/* Special allocation/deallocation functions in the global scope */
void* operator new(size_t size, const char* file, int line);
void* operator new[](size_t size, const char* file, int line);
void operator delete(void* ptr, const char* file, int line) _NOEXCEPT;
void operator delete[](void* ptr, const char* file, int line) _NOEXCEPT;

NVWA_NAMESPACE_BEGIN

/**
 * @def _DEBUG_NEW_REDEFINE_NEW
 *
 * Macro to indicate whether redefinition of \c new is wanted.  If one
 * wants to define one's own <code>operator new</code>, or to call
 * <code>operator new</code> directly, it should be defined to \c 0 to
 * alter the default behaviour.  Unless, of course, one is willing to
 * take the trouble to write something like:
 * @code
 * # ifdef new
 * #   define _NEW_REDEFINED
 * #   undef new
 * # endif
 *
 * // Code that uses new is here
 *
 * # ifdef _NEW_REDEFINED
 * #   ifdef DEBUG_NEW
 * #     define new DEBUG_NEW
 * #   endif
 * #   undef _NEW_REDEFINED
 * # endif
 * @endcode
 */
#ifndef _DEBUG_NEW_REDEFINE_NEW
#define _DEBUG_NEW_REDEFINE_NEW 1
#endif

/**
 * @def _DEBUG_NEW_TYPE
 *
 * Macro to indicate which variant of #DEBUG_NEW is wanted.  The
 * default value \c 1 allows the use of placement new (like
 * <code>%new(std::nothrow)</code>), but the verbose output (when
 * nvwa#new_verbose_flag is \c true) looks worse than some older
 * versions (no file/line information for allocations).  Define it
 * to \c 2 to revert to the old behaviour that records file and line
 * information directly on the call to <code>operator new</code>.
 */
#ifndef _DEBUG_NEW_TYPE
#define _DEBUG_NEW_TYPE 1
#endif

/**
 * Callback type for stack trace printing.
 *
 * @param fp          pointer to the output stream
 * @param stacktrace  pointer to the stack trace array (null-terminated)
 */
typedef void (*stacktrace_print_callback_t)(FILE* fp, void** stacktrace);

/**
 * Callback type for the leak whitelist function.  \a file, \a address,
 * and \a backtrace might be null depending on library configuration,
 * platform, and amount of runtime information available.  \a line can
 * be 0 when line number info is not available at runtime.
 *
 * @param file        null-terminated string of the file name
 * @param line        line number
 * @param addr        address of code where leakage happens
 * @param stacktrace  pointer to the stack trace array (null-terminated)
 * @return            \c true if the leak should be whitelisted;
 *                    \c false otherwise
 */
typedef bool (*leak_whitelist_callback_t)(char const* file, int line,
                                          void* addr, void** stacktrace);

/* Prototypes */
int check_leaks();
int check_mem_corruption();

/* Control variables */
extern bool new_autocheck_flag; // default to true: call check_leaks() on exit
extern bool new_verbose_flag;   // default to false: no verbose information
extern FILE* new_output_fp;     // default to stderr: output to console
extern const char* new_progname;// default to null; should be assigned argv[0]
extern stacktrace_print_callback_t stacktrace_print_callback;// default to null
extern leak_whitelist_callback_t leak_whitelist_callback;    // default to null

/**
 * @def DEBUG_NEW
 *
 * Macro to catch file/line information on allocation.  If
 * #_DEBUG_NEW_REDEFINE_NEW is \c 0, one can use this macro directly;
 * otherwise \c new will be defined to it, and one must use \c new
 * instead.
 */
# if _DEBUG_NEW_TYPE == 1
#   define DEBUG_NEW NVWA::debug_new_recorder(__FILE__, __LINE__) ->* new
# else
#   define DEBUG_NEW new(__FILE__, __LINE__)
# endif

# if _DEBUG_NEW_REDEFINE_NEW
#   define new DEBUG_NEW
# endif
# ifdef _DEBUG_NEW_EMULATE_MALLOC
#   include <stdlib.h>
#   ifdef new
#     define malloc(s) ((void*)(new char[s]))
#   else
#     define malloc(s) ((void*)(DEBUG_NEW char[s]))
#   endif
#   define free(p) delete[] (char*)(p)
# endif

/**
 * Recorder class to remember the call context.
 *
 * The idea comes from <a href="http://groups.google.com/group/comp.lang.c++.moderated/browse_thread/thread/7089382e3bc1c489/85f9107a1dc79ee9?#85f9107a1dc79ee9">Greg Herlihy's post</a> in comp.lang.c++.moderated.
 */
class debug_new_recorder
{
    const char* _M_file;
    const int   _M_line;
    void _M_process(void* ptr);

public:
    /**
     * Constructor to remember the call context.  The information will
     * be used in debug_new_recorder::operator->*.
     */
    debug_new_recorder(const char* file, int line)
        : _M_file(file), _M_line(line) {}
    /**
     * Operator to write the context information to memory.
     * <code>operator->*</code> is chosen because it has the right
     * precedence, it is rarely used, and it looks good: so people can
     * tell the special usage more quickly.
     */
    template <class _Tp> _Tp* operator->*(_Tp* ptr)
    { _M_process(ptr); return ptr; }

private:
    debug_new_recorder(const debug_new_recorder&) _DELETED;
    debug_new_recorder& operator=(const debug_new_recorder&) _DELETED;
};

/**
 * Counter class for on-exit leakage check.
 *
 * This technique is learnt from <em>The C++ Programming Language</em> by
 * Bjarne Stroustup.
 */
class debug_new_counter
{
    static int _S_count;
public:
    debug_new_counter();
    ~debug_new_counter();
};
/** Counting object for each file including debug_new.h. */
static debug_new_counter __debug_new_count;

NVWA_NAMESPACE_END

#endif // NVWA_DEBUG_NEW_H
