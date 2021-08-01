// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2021 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  debug_new.cpp
 *
 * Implementation of debug versions of new and delete to check leakage.
 *
 * @date  2021-08-01
 */

#include <new>                  // std::bad_alloc/nothrow_t
#include <assert.h>             // assert
#include <stdint.h>             // uint32_t
#include <stdio.h>              // fprintf/stderr
#include <stdlib.h>             // abort/malloc/free/posix_memalign
#include <string.h>             // strcpy/strncpy/sprintf
#include "_nvwa.h"              // NVWA macros

#if NVWA_UNIX
#include <alloca.h>             // alloca
#endif
#if NVWA_WIN32
#include <malloc.h>             // alloca/_aligned_malloc/_aligned_free
#endif

#if NVWA_LINUX || NVWA_APPLE
#include <execinfo.h>           // backtrace
#endif

#if NVWA_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>            // CaptureStackBackTrace
#endif

#include "fast_mutex.h"         // nvwa::fast_mutex

#undef  _DEBUG_NEW_EMULATE_MALLOC
#undef  _DEBUG_NEW_REDEFINE_NEW
/**
 * Macro to indicate whether redefinition of \c new is wanted.  Here it
 * is defined to \c 0 to disable the redefinition of \c new.
 */
#define _DEBUG_NEW_REDEFINE_NEW 0
#include "debug_new.h"

#if !_FAST_MUTEX_CHECK_INITIALIZATION && !defined(_NOTHREADS)
#error "_FAST_MUTEX_CHECK_INITIALIZATION not set: check_leaks may not work"
#endif

/**
 * @def _DEBUG_NEW_ALIGNMENT
 *
 * The alignment requirement of allocated memory blocks.  It must be a
 * power of two.  The value <code>sizeof(size_t) * 2</code> is a
 * reasonable guess, and agrees with \c __STDCPP_DEFAULT_NEW_ALIGNMENT__
 * on most platforms I have tested.  It may be smaller than the real
 * alignment, but must be bigger than \c sizeof(size_t) so that
 * nvwa#debug_new_recorder can detect misaligned pointer returned by
 * <code>new non-POD-type[size]</code>.
 */
#ifndef _DEBUG_NEW_ALIGNMENT
#ifdef __STDCPP_DEFAULT_NEW_ALIGNMENT__
#define _DEBUG_NEW_ALIGNMENT __STDCPP_DEFAULT_NEW_ALIGNMENT__
#else
#define _DEBUG_NEW_ALIGNMENT (sizeof(size_t) * 2)
#endif
#endif

/**
 * @def _DEBUG_NEW_CALLER_ADDRESS
 *
 * The expression to return the caller address.  nvwa#print_position will
 * later on use this address to print the position information of memory
 * operation points.
 */
#ifndef _DEBUG_NEW_CALLER_ADDRESS
#ifdef __GNUC__
#define _DEBUG_NEW_CALLER_ADDRESS __builtin_return_address(0)
#else
#define _DEBUG_NEW_CALLER_ADDRESS nullptr
#endif
#endif

/**
 * @def _DEBUG_NEW_ERROR_ACTION
 *
 * The action to take when an error occurs.  The default behaviour is to
 * call \e abort, unless \c _DEBUG_NEW_ERROR_CRASH is defined, in which
 * case a segmentation fault will be triggered instead (which can be
 * useful on platforms like Windows that do not generate a core dump
 * when \e abort is called).
 */
#ifndef _DEBUG_NEW_ERROR_ACTION
#ifndef _DEBUG_NEW_ERROR_CRASH
#define _DEBUG_NEW_ERROR_ACTION abort()
#else
#define _DEBUG_NEW_ERROR_ACTION        \
    do {                               \
        *(static_cast<char*>(0)) = 0;  \
        abort();                       \
    } while (0)
#endif
#endif

/**
 * @def _DEBUG_NEW_FILENAME_LEN
 *
 * The length of file name stored if greater than zero.  If it is zero,
 * only a const char pointer will be stored.  Currently the default
 * value is non-zero (thus to copy the file name) on non-Windows
 * platforms, because I once found that the exit leakage check could not
 * access the address of the file name on Linux (in my case, a core dump
 * occurred when check_leaks tried to access the file name in a shared
 * library after a \c SIGINT).
 */
#ifndef _DEBUG_NEW_FILENAME_LEN
#if NVWA_WINDOWS
#define _DEBUG_NEW_FILENAME_LEN 0
#else
#define _DEBUG_NEW_FILENAME_LEN 44
#endif
#endif

/**
 * @def _DEBUG_NEW_PROGNAME
 *
 * The program (executable) name to be set at compile time.  It is
 * better to assign the full program path to nvwa#new_progname in \e main
 * (at run time) than to use this (compile-time) macro, but this macro
 * serves well as a quick hack.  Note also that double quotation marks
 * need to be used around the program name, i.e., one should specify a
 * command-line option like <code>-D_DEBUG_NEW_PROGNAME=\\"a.out\"</code>
 * in \e bash, or <code>-D_DEBUG_NEW_PROGNAME=\\"a.exe\"</code> in the
 * Windows command prompt.
 */
#ifndef _DEBUG_NEW_PROGNAME
#define _DEBUG_NEW_PROGNAME nullptr
#endif

/**
 * @def _DEBUG_NEW_REMEMBER_STACK_TRACE
 *
 * Macro to indicate whether stack traces of allocations should be
 * included in NVWA allocation information and be printed while leaks
 * are reported.  Useful to pinpoint leaks caused by strdup and other
 * custom allocation functions.  It is also very helpful in filtering
 * out false positives caused by internal STL/C runtime operations.  It
 * is off by default because it is quite memory heavy.  Set it to \c 1
 * to make all allocations have stack trace attached, or set to \c 2 to
 * make only allocations that lack calling source code information (file
 * and line) have the stack trace attached.
 */
#ifndef _DEBUG_NEW_REMEMBER_STACK_TRACE
#define _DEBUG_NEW_REMEMBER_STACK_TRACE 0
#endif

/**
 * @def _DEBUG_NEW_TAILCHECK
 *
 * Macro to indicate whether a writing-past-end check will be performed.
 * Define it to a positive integer as the number of padding bytes at the
 * end of a memory block for checking.
 */
#ifndef _DEBUG_NEW_TAILCHECK
#define _DEBUG_NEW_TAILCHECK 0
#endif

/**
 * @def _DEBUG_NEW_TAILCHECK_CHAR
 *
 * Value of the padding bytes at the end of a memory block.
 */
#ifndef _DEBUG_NEW_TAILCHECK_CHAR
#define _DEBUG_NEW_TAILCHECK_CHAR 0xCC
#endif

/**
 * @def _DEBUG_NEW_USE_ADDR2LINE
 *
 * Whether to use \e addr2line to convert a caller address to file/line
 * information.  Defining it to a non-zero value will enable the
 * conversion (automatically done if GCC is detected).  Defining it to
 * zero will disable the conversion.
 */
#ifndef _DEBUG_NEW_USE_ADDR2LINE
#ifdef __GNUC__
#define _DEBUG_NEW_USE_ADDR2LINE 1
#else
#define _DEBUG_NEW_USE_ADDR2LINE 0
#endif
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4074)  // #pragma init_seg(compiler) used
#if _MSC_VER >= 1400            // Visual Studio 2005 or later
#pragma warning(disable: 4996)  // Use the `unsafe' strncpy
#endif
#pragma init_seg(compiler)
#endif

NVWA_NAMESPACE_BEGIN

/**
 * Structure to store the position information where \c new occurs.
 */
struct new_ptr_list_t {
    new_ptr_list_t* next;       ///< Pointer to the next memory block
    new_ptr_list_t* prev;       ///< Pointer to the previous memory block
    size_t          size;       ///< Size of the memory block
    union {
#if _DEBUG_NEW_FILENAME_LEN == 0
    const char*     file;       ///< Pointer to the file name of the caller
#else
    char            file[_DEBUG_NEW_FILENAME_LEN]; ///< File name of the caller
#endif
    void*           addr;       ///< Address of the caller to \e new
    };
    uint32_t        head_size;  ///< Size of this struct, aligned
    uint32_t        line   :31; ///< Line number of the caller; or \c 0
    uint32_t        is_array:1; ///< Non-zero iff <em>new[]</em> is used
#if _DEBUG_NEW_REMEMBER_STACK_TRACE
    void**          stacktrace; ///< Pointer to stack trace information
#endif
    uint32_t        magic;      ///< Magic number for error detection
};

enum is_array_t {
    alloc_is_not_array,
    alloc_is_array
};

/**
 * Flag to control whether nvwa#check_leaks will be automatically called
 * on program exit.
 */
bool new_autocheck_flag = true;

/**
 * Flag to control whether verbose messages are output.
 */
bool new_verbose_flag = false;

/**
 * Pointer to the output stream.  The default output is \e stderr, and
 * one may change it to a user stream if needed (say, #new_verbose_flag
 * is \c true and there are a lot of (de)allocations).
 */
FILE* new_output_fp = stderr;

/**
 * Pointer to the program name.  Its initial value is the macro
 * #_DEBUG_NEW_PROGNAME.  You should try to assign the program path to
 * it early in your application.  Assigning <code>argv[0]</code> to it
 * in \e main is one way.  If you use \e bash or \e ksh (or similar),
 * the following statement is probably what you want:
 * `<code>new_progname = getenv("_");</code>'.
 */
const char* new_progname = _DEBUG_NEW_PROGNAME;

/**
 * Pointer to the callback used to print the stack backtrace in case of
 * a memory problem.  A null value causes the default stack trace
 * printing routine to be used.
 */
stacktrace_print_callback_t stacktrace_print_callback = nullptr;

/**
 * Pointer to the callback used to filter out false positives from leak
 * reports.  A null value means the lack of filtering.
 */
leak_whitelist_callback_t leak_whitelist_callback = nullptr;

namespace {

/**
 * Gets the aligned size of a memory block.
 *
 * @param s          size of the memory block
 * @param alignment  alignment requested
 * @return           aligned size
 */
inline constexpr uint32_t align(size_t s,
                                size_t alignment = _DEBUG_NEW_ALIGNMENT)
{
    // 32 bits are enough for alignments
    return static_cast<uint32_t>((s + alignment - 1) & ~(alignment - 1));
}

/**
 * Definition of the constant magic number used for error detection.
 */
constexpr uint32_t DEBUG_NEW_MAGIC = 0x4442474E;

/**
 * The extra memory allocated by <code>operator new</code>.
 */
constexpr uint32_t ALIGNED_LIST_ITEM_SIZE = align(sizeof(new_ptr_list_t));

/**
 * List of all new'd pointers.
 */
new_ptr_list_t new_ptr_list = {
    &new_ptr_list,
    &new_ptr_list,
    0,
    {
#if _DEBUG_NEW_FILENAME_LEN == 0
        nullptr
#else
        ""
#endif
    },
    0,
    0,
    0,
#if _DEBUG_NEW_REMEMBER_STACK_TRACE
    nullptr,
#endif
    DEBUG_NEW_MAGIC
};

/**
 * The mutex guard to protect simultaneous access to the pointer list.
 */
fast_mutex new_ptr_lock;

/**
 * The mutex guard to protect simultaneous output to #new_output_fp.
 */
fast_mutex new_output_lock;

/**
 * Allocated memory in bytes.
 */
size_t current_mem_alloc = 0;

/**
 * Accumulated count of total memory allocations.
 */
size_t total_mem_alloc_cnt_accum = 0;

#if _DEBUG_NEW_USE_ADDR2LINE
/**
 * Tries printing the position information from an instruction address.
 * This is the version that uses \e addr2line.
 *
 * @param addr  the instruction address to convert and print
 * @return      \c true if the address is converted successfully (and
 *              the result is printed); \c false if no useful
 *              information is got (and nothing is printed)
 */
bool print_position_from_addr(const void* addr)
{
    static const void* last_addr = nullptr;
    static char last_info[256] = "";
    if (addr == last_addr) {
        if (last_info[0] == '\0') {
            return false;
        }
        fprintf(new_output_fp, "%s", last_info);
        return true;
    }
    if (new_progname) {
#if NVWA_APPLE
        const char addr2line_cmd[] = "atos -o ";
#else
        const char addr2line_cmd[] = "addr2line -e ";
#endif

#if NVWA_WINDOWS
        const int  exeext_len = 4;
#else
        const int  exeext_len = 0;
#endif

#if NVWA_UNIX && !NVWA_CYGWIN
        const char ignore_err[] = " 2>/dev/null";
#elif NVWA_CYGWIN || \
        (NVWA_WIN32 && defined(WINVER) && WINVER >= 0x0500)
        const char ignore_err[] = " 2>nul";
#else
        const char ignore_err[] = "";
#endif
        auto cmd = static_cast<char*>(alloca(strlen(new_progname)
                                             + exeext_len
                                             + sizeof addr2line_cmd - 1
                                             + sizeof ignore_err - 1
                                             + sizeof(void*) * 2
                                             + 4 /* SP + "0x" + null */));
        strcpy(cmd, addr2line_cmd);
        strcpy(cmd + sizeof addr2line_cmd - 1, new_progname);
        size_t len = strlen(cmd);
#if NVWA_WINDOWS
        if (len <= 4 || (strcmp(cmd + len - 4, ".exe") != 0 &&
                         strcmp(cmd + len - 4, ".EXE") != 0)) {
            strcpy(cmd + len, ".exe");
            len += 4;
        }
#endif
        sprintf(cmd + len, " %p%s", addr, ignore_err);
        FILE* fp = popen(cmd, "r");
        if (fp) {
            char buffer[sizeof last_info] = "";
            len = 0;
            if (fgets(buffer, sizeof buffer, fp)) {
                len = strlen(buffer);
                if (buffer[len - 1] == '\n') {
                    buffer[--len] = '\0';
                }
            }
            int res = pclose(fp);
            // Display the file/line information only if the command
            // is executed successfully and the output points to a
            // valid position, but the result will be cached if only
            // the command is executed successfully.
            if (res == 0 && len > 0) {
                last_addr = addr;
                if (buffer[len - 1] == '0' && buffer[len - 2] == ':') {
                    last_info[0] = '\0';
                } else {
                    fprintf(new_output_fp, "%s", buffer);
                    strcpy(last_info, buffer);
                    return true;
                }
            }
        }
    }
    return false;
}
#else
/**
 * Tries printing the position information from an instruction address.
 * This is the stub version that does nothing at all.
 *
 * @return      \c false always
 */
bool print_position_from_addr(const void*)
{
    return false;
}
#endif // _DEBUG_NEW_USE_ADDR2LINE

/**
 * Prints the position information of a memory operation point.  When \c
 * _DEBUG_NEW_USE_ADDR2LINE is defined to a non-zero value, this
 * function will try to convert a given caller address to file/line
 * information with \e addr2line.
 *
 * @param ptr   source file name if \e line is non-zero; caller address
 *              otherwise
 * @param line  source line number if non-zero; indication that \e ptr
 *              is the caller address otherwise
 */
void print_position(const void* ptr, int line)
{
    if (line != 0) {               // Is file/line information present?
        fprintf(new_output_fp, "%s:%d", static_cast<const char*>(ptr), line);
    } else if (ptr != nullptr) {   // Is caller address present?
        if (!print_position_from_addr(ptr)) { // Fail to get source position?
            fprintf(new_output_fp, "%p", ptr);
        }
    } else {                       // No information is present
        fprintf(new_output_fp, "<Unknown>");
    }
}

#if _DEBUG_NEW_REMEMBER_STACK_TRACE
/**
 * Prints the stack backtrace.
 *
 * When nvwa#stacktrace_print_callback is not null, it is used for
 * printing the stacktrace items.  Default implementation of call stack
 * printing is very spartan&mdash;only stack frame pointers are
 * printed&mdash;but even that output is still useful.  Just do address
 * lookup in LLDB etc.
 *
 * @param stacktrace  pointer to the stack trace array
 */
void print_stacktrace(void** stacktrace)
{
    if (stacktrace_print_callback == nullptr) {
        fprintf(new_output_fp, "Stack backtrace:\n");
        for (size_t i = 0; stacktrace[i] != nullptr; ++i) {
            fprintf(new_output_fp, "%p\n", stacktrace[i]);
        }
    } else {
        stacktrace_print_callback(new_output_fp, stacktrace);
    }
}
#endif

/**
 * Checks whether a leak should be ignored.  Its runtime performance
 * depends on the callback nvwa#leak_whitelist_callback.
 *
 * @param ptr  pointer to a new_ptr_list_t struct
 * @return     \c true if the leak should be whitelisted; \c false
 *             otherwise
 */
bool is_leak_whitelisted(new_ptr_list_t* ptr)
{
    if (leak_whitelist_callback == nullptr) {
        return false;
    }

    char const* file = ptr->line != 0 ? ptr->file : nullptr;
    int line = ptr->line;
    void* addr = ptr->line == 0 ? ptr->addr : nullptr;
#if _DEBUG_NEW_REMEMBER_STACK_TRACE
    void** stacktrace = ptr->stacktrace;
#else
    void** stacktrace = nullptr;
#endif

    return leak_whitelist_callback(file, line, addr, stacktrace);
}

#if _DEBUG_NEW_TAILCHECK
/**
 * Checks whether the padding bytes at the end of a memory block is
 * tampered with.
 *
 * @param ptr  pointer to a new_ptr_list_t struct
 * @return     \c true if the padding bytes are untouched; \c false
 *             otherwise
 */
bool check_tail(new_ptr_list_t* ptr)
{
    auto const tail_ptr = reinterpret_cast<const unsigned char*>(ptr) +
                          ptr->size + ptr->head_size;
    for (int i = 0; i < _DEBUG_NEW_TAILCHECK; ++i) {
        if (tail_ptr[i] != _DEBUG_NEW_TAILCHECK_CHAR) {
            return false;
        }
    }
    return true;
}
#endif

/**
 * Converts the user-provided pointer to the real allocation address, in
 * type \c new_ptr_list_t*.  In an expression <code>new non-POD-type[size]
 * </code>, the pointer returned is not the pointer returned by <code>
 * operator new[]</code>, but offset to leave room for the size.  It
 * needs to be compensated for.
 *
 * @param usr_ptr    pointer returned by a new-expression
 * @param alignment  alignment value for this allocation
 * @return           a valid pointer if an aligned pointer is got;
 *                   \c nullptr otherwise
 */
new_ptr_list_t* convert_user_ptr(void* usr_ptr, size_t alignment)
{
    auto offset = static_cast<char*>(usr_ptr) - static_cast<char*>(nullptr);
    auto adjusted_ptr = static_cast<char*>(usr_ptr);
    bool is_adjusted = false;

    // Check alignment first
    if (offset % alignment != 0) {
        offset -= sizeof(size_t);
        if (offset % alignment != 0) {
            return nullptr;
        }
        adjusted_ptr = static_cast<char*>(usr_ptr) - sizeof(size_t);
        is_adjusted = true;
    }
    auto ptr = reinterpret_cast<new_ptr_list_t*>(
        adjusted_ptr - align(sizeof(new_ptr_list_t), alignment));
    if (ptr->magic == DEBUG_NEW_MAGIC && (!is_adjusted || ptr->is_array)) {
        return ptr;
    }

    // Aligned new[] allocates alignment extra space for the array size
    if (!is_adjusted && alignment > _DEBUG_NEW_ALIGNMENT) {
        ptr = reinterpret_cast<new_ptr_list_t*>(
            reinterpret_cast<char*>(ptr) - alignment);
        is_adjusted = true;
    }
    if (ptr->magic == DEBUG_NEW_MAGIC && (!is_adjusted || ptr->is_array)) {
        return ptr;
    }

    return nullptr;
}

/**
 * Allocates memory for debug_new.  Memory allocations are aligned.
 *
 * @param size       size of memory to allocate
 * @param alignment  alignment requested
 * @return           pointer to allocated memory if successful;
 *                   \c nullptr other wise
 */
void* debug_new_alloc(size_t size, size_t alignment = _DEBUG_NEW_ALIGNMENT)
{
#if NVWA_WIN32
    return _aligned_malloc(size, alignment);
#elif NVWA_UNIX
    void* memptr;
    int result = posix_memalign(&memptr, alignment, size);
    if (result == 0) {
        return memptr;
    } else {
        return nullptr;
    }
#else
    // No alignment guarantees on non-Windows, non-Unix platforms
    return malloc(size);
#endif
}

/**
 * Deallocates memory for debug_new.
 *
 * @param ptr  pointer to the memory to deallocate
 */
void debug_new_free(void* ptr)
{
#if NVWA_WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

/**
 * Allocates memory and initializes control data.
 *
 * @param size      size of the required memory block
 * @param file      null-terminated string of the file name
 * @param line      line number
 * @param is_array  flag indicating whether it is invoked by a
 *                  <code>new[]</code> call
 * @return          pointer to the user-requested memory area;
 *                  \c nullptr if memory allocation is not successful
 */
void* alloc_mem(size_t size, const char* file, int line,
                is_array_t is_array,
                size_t alignment = _DEBUG_NEW_ALIGNMENT)
{
#if _DEBUG_NEW_TYPE == 1
    static_assert(_DEBUG_NEW_ALIGNMENT >= sizeof(size_t) * 2,
                  "Alignment is too small");
#endif
    static_assert((_DEBUG_NEW_ALIGNMENT & (_DEBUG_NEW_ALIGNMENT - 1)) == 0,
                  "Alignment must be power of two");
    static_assert(_DEBUG_NEW_TAILCHECK >= 0, "Invalid tail check length");
    assert(line >= 0);
    assert(alignment >= _DEBUG_NEW_ALIGNMENT);

    uint32_t aligned_list_item_size = align(sizeof(new_ptr_list_t), alignment);
    size_t s = size + aligned_list_item_size + _DEBUG_NEW_TAILCHECK;
    auto ptr = static_cast<new_ptr_list_t*>(debug_new_alloc(s, alignment));
    if (ptr == nullptr) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "Out of memory when allocating %zu bytes\n",
                size);
        fflush(new_output_fp);
        _DEBUG_NEW_ERROR_ACTION;
    }
    auto usr_ptr = reinterpret_cast<char*>(ptr) + aligned_list_item_size;
#if _DEBUG_NEW_FILENAME_LEN == 0
    ptr->file = file;
#else
    if (line) {
        strncpy(ptr->file, file, _DEBUG_NEW_FILENAME_LEN - 1)
                [_DEBUG_NEW_FILENAME_LEN - 1] = '\0';
    } else {
        ptr->addr = const_cast<char*>(file);
    }
#endif
    ptr->line = line;
#if _DEBUG_NEW_REMEMBER_STACK_TRACE
    ptr->stacktrace = nullptr;

#if _DEBUG_NEW_REMEMBER_STACK_TRACE == 2
    if (line == 0)
#endif
    {
        void* buffer [255];
        size_t buffer_length = sizeof(buffer) / sizeof(*buffer);

#if NVWA_UNIX
        int stacktrace_length = backtrace(buffer, int(buffer_length));
#endif

#if NVWA_WINDOWS
        USHORT stacktrace_length = CaptureStackBackTrace(
            0, DWORD(buffer_length), buffer, nullptr);
#endif

        size_t stacktrace_size = stacktrace_length * sizeof(void*);
        ptr->stacktrace =
            static_cast<void**>(malloc(stacktrace_size + sizeof(void*)));

        if (ptr->stacktrace != nullptr) {
            memcpy(ptr->stacktrace, buffer, stacktrace_size);
            ptr->stacktrace[stacktrace_length] = nullptr;
        }
    }
#endif
    ptr->is_array = is_array;
    ptr->size = size;
    ptr->head_size = aligned_list_item_size;
    ptr->magic = DEBUG_NEW_MAGIC;
    {
        fast_mutex_autolock lock(new_ptr_lock);
        ptr->prev = new_ptr_list.prev;
        ptr->next = &new_ptr_list;
        new_ptr_list.prev->next = ptr;
        new_ptr_list.prev = ptr;
    }
#if _DEBUG_NEW_TAILCHECK
    memset(usr_ptr + size, _DEBUG_NEW_TAILCHECK_CHAR, _DEBUG_NEW_TAILCHECK);
#endif
    if (new_verbose_flag) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "new%s: allocated %p (size %zu, ",
                is_array ? "[]" : "", usr_ptr, size);
        if (line != 0) {
            print_position(ptr->file, ptr->line);
        } else {
            print_position(ptr->addr, ptr->line);
        }
        fprintf(new_output_fp, ")\n");
    }
    current_mem_alloc += size;
    total_mem_alloc_cnt_accum++;
    return usr_ptr;
}

/**
 * Frees memory and adjusts pointers.
 *
 * @param usr_ptr   pointer returned by a new-expression
 * @param addr      pointer to the caller
 * @param is_array  flag indicating whether it is invoked by a
 *                  <code>delete[]</code> call
 */
void free_pointer(void* usr_ptr, void* addr, is_array_t is_array,
                  size_t alignment = _DEBUG_NEW_ALIGNMENT)
{
    assert(alignment >= _DEBUG_NEW_ALIGNMENT);
    if (usr_ptr == nullptr) {
        return;
    }

    auto ptr = convert_user_ptr(usr_ptr, alignment);
    if (ptr == nullptr) {
        {
            fast_mutex_autolock lock(new_output_lock);
            fprintf(new_output_fp, "delete%s: invalid pointer %p (",
                    is_array ? "[]" : "", usr_ptr);
            print_position(addr, 0);
            fprintf(new_output_fp, ")\n");
        }
        check_mem_corruption();
        fflush(new_output_fp);
        _DEBUG_NEW_ERROR_ACTION;
    }
    if (is_array != ptr->is_array) {
        const char* msg;
        if (is_array) {
            msg = "delete[] after new";
        } else {
            msg = "delete after new[]";
        }
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "%s: pointer %p (size %zu)\n\tat ",
                msg, usr_ptr, ptr->size);
        print_position(addr, 0);
        fprintf(new_output_fp, "\n\toriginally allocated at ");
        if (ptr->line != 0) {
            print_position(ptr->file, ptr->line);
        } else {
            print_position(ptr->addr, ptr->line);
        }
        fprintf(new_output_fp, "\n");
        fflush(new_output_fp);
        _DEBUG_NEW_ERROR_ACTION;
    }
#if _DEBUG_NEW_TAILCHECK
    if (!check_tail(ptr)) {
        check_mem_corruption();
        fflush(new_output_fp);
        _DEBUG_NEW_ERROR_ACTION;
    }
#endif
    {
        fast_mutex_autolock lock(new_ptr_lock);
        current_mem_alloc -= ptr->size;
        ptr->magic = 0;
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
    }
    if (new_verbose_flag) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "delete%s: freed %p (size %zu, %zu bytes still allocated)\n",
                is_array ? "[]" : "", usr_ptr, ptr->size, current_mem_alloc);
    }
#if _DEBUG_NEW_REMEMBER_STACK_TRACE
    free(ptr->stacktrace);
#endif
    debug_new_free(ptr);
}

} /* unnamed namespace */

/**
 * Checks for memory leaks.
 *
 * @return  zero if no leakage is found; the number of leaks otherwise
 */
int check_leaks()
{
    int leak_cnt = 0;
    int whitelisted_leak_cnt = 0;
    fast_mutex_autolock lock_ptr(new_ptr_lock);
    fast_mutex_autolock lock_output(new_output_lock);
    new_ptr_list_t* ptr = new_ptr_list.next;

    while (ptr != &new_ptr_list) {
        auto usr_ptr =
            reinterpret_cast<const char*>(ptr) + ALIGNED_LIST_ITEM_SIZE;
        if (ptr->magic != DEBUG_NEW_MAGIC) {
            fprintf(new_output_fp,
                    "warning: heap data corrupt near %p\n",
                    usr_ptr);
        } else {
            // Adjust usr_ptr after the basic sanity check
            usr_ptr = reinterpret_cast<const char*>(ptr) + ptr->head_size;
        }
#if _DEBUG_NEW_TAILCHECK
        if (!check_tail(ptr)) {
            fprintf(new_output_fp,
                    "warning: overwritten past end of object at %p\n",
                    usr_ptr);
        }
#endif

        if (is_leak_whitelisted(ptr)) {
            ++whitelisted_leak_cnt;
        } else {
            fprintf(new_output_fp,
                    "Leaked object at %p (size %zu, ",
                    usr_ptr, ptr->size);

            if (ptr->line != 0) {
                print_position(ptr->file, ptr->line);
            } else {
                print_position(ptr->addr, ptr->line);
            }

            fprintf(new_output_fp, ")\n");

#if _DEBUG_NEW_REMEMBER_STACK_TRACE
            if (ptr->stacktrace != nullptr) {
                print_stacktrace(ptr->stacktrace);
            }
#endif
        }

        ptr = ptr->next;
        ++leak_cnt;
    }
    if (new_verbose_flag || leak_cnt) {
        if (whitelisted_leak_cnt > 0) {
            fprintf(new_output_fp, "*** %d leaks found (%d whitelisted)\n",
                leak_cnt, whitelisted_leak_cnt);
        } else {
            fprintf(new_output_fp, "*** %d leaks found\n", leak_cnt);
        }
    }

    return leak_cnt;
}

/**
 * Checks for heap corruption.
 *
 * @return  zero if no problem is found; the number of found memory
 *          corruptions otherwise
 */
int check_mem_corruption()
{
    int corrupt_cnt = 0;
    fast_mutex_autolock lock_ptr(new_ptr_lock);
    fast_mutex_autolock lock_output(new_output_lock);
    fprintf(new_output_fp, "*** Checking for memory corruption: START\n");
    for (new_ptr_list_t* ptr = new_ptr_list.next;
            ptr != &new_ptr_list;
            ptr = ptr->next) {
        auto usr_ptr =
            reinterpret_cast<const char*>(ptr) + ALIGNED_LIST_ITEM_SIZE;
        if (ptr->magic == DEBUG_NEW_MAGIC
#if _DEBUG_NEW_TAILCHECK
            && check_tail(ptr)
#endif
        ) {
            continue;
        }
#if _DEBUG_NEW_TAILCHECK
        if (ptr->magic != DEBUG_NEW_MAGIC) {
#endif
            fprintf(new_output_fp,
                    "Heap data corrupt near %p (size %zu, ",
                    usr_ptr, ptr->size);
#if _DEBUG_NEW_TAILCHECK
        } else {
            // Adjust usr_ptr after the basic sanity check
            usr_ptr = reinterpret_cast<const char*>(ptr) + ptr->head_size;
            fprintf(new_output_fp,
                    "Overwritten past end of object at %p (size %zu, ",
                    usr_ptr, ptr->size);
        }
#endif
        if (ptr->line != 0) {
            print_position(ptr->file, ptr->line);
        } else {
            print_position(ptr->addr, ptr->line);
        }
        fprintf(new_output_fp, ")\n");

#if _DEBUG_NEW_REMEMBER_STACK_TRACE
        if (ptr->stacktrace != nullptr)
            print_stacktrace(ptr->stacktrace);
#endif

        ++corrupt_cnt;
    }
    fprintf(new_output_fp, "*** Checking for memory corruption: %d FOUND\n",
            corrupt_cnt);
    return corrupt_cnt;
}

/**
 * Gets the current allocated memory in bytes.
 *
 * @return  bytes of currently allocated memory
 */
size_t get_current_mem_alloc()
{
    return current_mem_alloc;
}

/**
 * Gets the total memory allocation count.
 *
 * @return  count of calls to the allocation function
 */
size_t get_total_mem_alloc_cnt()
{
    return total_mem_alloc_cnt_accum;
}

/**
 * Processes the allocated memory and inserts file/line informatin.
 * It will only be done when it can ensure the memory is allocated by
 * one of our operator new variants.
 *
 * @param usr_ptr  pointer returned by a new-expression
 */
void debug_new_recorder::_M_process(void* usr_ptr)
{
    if (usr_ptr == nullptr) {
        return;
    }

    auto ptr = convert_user_ptr(usr_ptr, _DEBUG_NEW_ALIGNMENT);
    if (ptr == nullptr || ptr->line != 0) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "warning: debug_new used with placement new (%s:%d)\n",
                _M_file, _M_line);
        return;
    }
    if (new_verbose_flag) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "info: pointer %p allocated from %s:%d\n",
                usr_ptr, _M_file, _M_line);
    }
#if _DEBUG_NEW_FILENAME_LEN == 0
    ptr->file = _M_file;
#else
    strncpy(ptr->file, _M_file, _DEBUG_NEW_FILENAME_LEN - 1)
            [_DEBUG_NEW_FILENAME_LEN - 1] = '\0';
#endif
    ptr->line = _M_line;
#if _DEBUG_NEW_REMEMBER_STACK_TRACE == 2
    free(ptr->stacktrace);
    ptr->stacktrace = nullptr;
#endif
}

/**
 * Count of source files that use debug_new.
 */
int debug_new_counter::_S_count = 0;

/**
 * Constructor to increment the count.
 */
debug_new_counter::debug_new_counter()
{
    ++_S_count;
}

/**
 * Destructor to decrement the count.  When the count is zero,
 * nvwa#check_leaks will be called.
 */
debug_new_counter::~debug_new_counter()
{
    if (--_S_count == 0 && new_autocheck_flag) {
        if (check_leaks()) {
            new_verbose_flag = true;
        }
    }
}

NVWA_NAMESPACE_END

#if NVWA_USE_NAMESPACE
using namespace nvwa;
#endif // NVWA_USE_NAMESPACE

/**
 * Allocates memory with file/line information.
 *
 * @param size  size of the required memory block
 * @param file  null-terminated string of the file name
 * @param line  line number
 * @return      pointer to the memory allocated
 * @throw bad_alloc memory is insufficient
 */
void* operator new(size_t size, const char* file, int line)
{
    void* ptr = alloc_mem(size, file, line, alloc_is_not_array);
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

/**
 * Allocates array memory with file/line information.
 *
 * @param size  size of the required memory block
 * @param file  null-terminated string of the file name
 * @param line  line number
 * @return      pointer to the memory allocated
 * @throw bad_alloc memory is insufficient
 */
void* operator new[](size_t size, const char* file, int line)
{
    void* ptr = alloc_mem(size, file, line, alloc_is_array);
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

/**
 * Allocates memory without file/line information.
 *
 * @param size  size of the required memory block
 * @return      pointer to the memory allocated
 * @throw bad_alloc memory is insufficient
 */
void* operator new(size_t size)
{
    return operator new(size,
                        static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0);
}

/**
 * Allocates array memory without file/line information.
 *
 * @param size  size of the required memory block
 * @return      pointer to the memory allocated
 * @throw bad_alloc memory is insufficient
 */
void* operator new[](size_t size)
{
    return operator new[](size,
                          static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0);
}

/**
 * Allocates memory with no-throw guarantee.
 *
 * @param size  size of the required memory block
 * @return      pointer to the memory allocated; or null if memory is
 *              insufficient
 */
void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return alloc_mem(size, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0,
                     alloc_is_not_array);
}

/**
 * Allocates array memory with no-throw guarantee.
 *
 * @param size  size of the required memory block
 * @return      pointer to the memory allocated; or null if memory is
 *              insufficient
 */
void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return alloc_mem(size, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0,
                     alloc_is_array);
}

/**
 * Deallocates memory.
 *
 * @param ptr  pointer to the previously allocated memory
 */
void operator delete(void* ptr) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_not_array);
}

/**
 * Deallocates array memory.
 *
 * @param ptr  pointer to the previously allocated memory
 */
void operator delete[](void* ptr) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_array);
}

#if __cplusplus >= 201402L
// GCC under C++14 wants these definitions

void operator delete(void* ptr, size_t) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_not_array);
}

void operator delete[](void* ptr, size_t) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_array);
}
#endif

/**
 * Placement deallocation function.
 *
 * @param ptr   pointer to the previously allocated memory
 * @param file  null-terminated string of the file name
 * @param line  line number
 *
 * @see   https://eel.is/c++draft/expr.new
 * @see   http://wyw.dcweb.cn/leakage.htm
 */
void operator delete(void* ptr, const char* file, int line) noexcept
{
    if (new_verbose_flag) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "info: exception thrown on initializing object at %p (",
                ptr);
        print_position(file, line);
        fprintf(new_output_fp, ")\n");
    }
    operator delete(ptr);
}

/**
 * Placement deallocation function.
 *
 * @param ptr   pointer to the previously allocated memory
 * @param file  null-terminated string of the file name
 * @param line  line number
 */
void operator delete[](void* ptr, const char* file, int line) noexcept
{
    if (new_verbose_flag) {
        fast_mutex_autolock lock(new_output_lock);
        fprintf(new_output_fp,
                "info: exception thrown on initializing objects at %p (",
                ptr);
        print_position(file, line);
        fprintf(new_output_fp, ")\n");
    }
    operator delete[](ptr);
}

/**
 * Placement deallocation function.
 *
 * @param ptr  pointer to the previously allocated memory
 */
void operator delete(void* ptr, const std::nothrow_t&) noexcept
{
    operator delete(ptr, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0);
}

/**
 * Placement deallocation function.
 *
 * @param ptr  pointer to the previously allocated memory
 */
void operator delete[](void* ptr, const std::nothrow_t&) noexcept
{
    operator delete[](ptr, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0);
}

#if NVWA_SUPPORTS_ALIGNED_NEW
void* operator new(size_t size, std::align_val_t align_val,
                   const char* file, int line)
{
    void* ptr =
        alloc_mem(size, file, line, alloc_is_not_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new[](size_t size, std::align_val_t align_val,
                     const char* file, int line)
{
    void* ptr =
        alloc_mem(size, file, line, alloc_is_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new(size_t size, std::align_val_t align_val)
{
    void* ptr =
        alloc_mem(size, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0,
                  alloc_is_not_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new[](size_t size, std::align_val_t align_val)
{
    void* ptr =
        alloc_mem(size, static_cast<char*>(_DEBUG_NEW_CALLER_ADDRESS), 0,
                  alloc_is_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void operator delete(void* ptr, std::align_val_t align_val) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_not_array,
                 size_t(align_val));
}

void operator delete[](void* ptr, std::align_val_t align_val) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_array,
                 size_t(align_val));
}

void operator delete(void* ptr, size_t, std::align_val_t align_val) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_not_array,
                 size_t(align_val));
}

void operator delete[](void* ptr, size_t, std::align_val_t align_val) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_array,
                 size_t(align_val));
}

void operator delete(void* ptr, std::align_val_t align_val,
                     const std::nothrow_t&) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_not_array,
                 size_t(align_val));
}

void operator delete[](void* ptr, std::align_val_t align_val,
                       const std::nothrow_t&) noexcept
{
    free_pointer(ptr, _DEBUG_NEW_CALLER_ADDRESS, alloc_is_array,
                 size_t(align_val));
}
#endif

// This is to make Doxygen happy
#undef _DEBUG_NEW_REMEMBER_STACK_TRACE
#define _DEBUG_NEW_REMEMBER_STACK_TRACE 0
