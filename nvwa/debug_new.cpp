// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004 Wu Yongwei <adah at users dot sourceforge dot net>
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
 *    not claim that you wrote the original software. If you use this
 *    software in a product, an acknowledgment in the product
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
 * @file    debug_new.cpp
 *
 * Implementation of debug versions of new and delete to check leakage.
 *
 * @version 2.9, 2004/12/08
 * @author  Wu Yongwei
 *
 */

#include <new>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include "fast_mutex.h"

#if !_FAST_MUTEX_CHECK_INITIALIZATION && !defined(_NOTHREADS)
#error "_FAST_MUTEX_CHECK_INITIALIZATION not set: check_leaks may not work"
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
#define _DEBUG_NEW_ERROR_ACTION do { *((char*)0) = 0; abort(); } while (0)
#endif
#endif

/**
 * @def _DEBUG_NEW_FILENAME_LEN
 *
 * The length of file name stored if greater than zero.  If it is zero,
 * only a const char pointer will be stored.  Currently the default
 * behaviour is to copy the file name, because I found that the exit
 * leakage check cannot access the address of the file name sometimes
 * (in my case, a core dump will occur when trying to access the file
 * name in a shared library after a \c SIGINT).  If the default value is
 * too small for you, try defining it to \c 52, which makes the size of
 * new_ptr_list_t 64 (it is 32 by default).
 */
#ifndef _DEBUG_NEW_FILENAME_LEN
#define _DEBUG_NEW_FILENAME_LEN  20
#endif
#if _DEBUG_NEW_FILENAME_LEN > 0
#include <string.h>
#endif

/**
 * @def _DEBUG_NEW_HASHTABLESIZE
 *
 * The size of the hash bucket for the table to store pointers to
 * allocated memory.  To ensure good performance, always make it a power
 * of two.
 */
#ifndef _DEBUG_NEW_HASHTABLESIZE
#define _DEBUG_NEW_HASHTABLESIZE 16384
#endif

/**
 * @def _DEBUG_NEW_HASH
 *
 * The hash function for the pointers.  This one has good performance in
 * test for me.
 */
#ifndef _DEBUG_NEW_HASH
#define _DEBUG_NEW_HASH(p) (((unsigned)(p) >> 8) % _DEBUG_NEW_HASHTABLESIZE)
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4073)  // #pragma init_seg(lib) used
#pragma warning(disable: 4290)  // C++ exception specification ignored
#pragma warning(disable: 4311)  // 64-bit porting warning for pointer cast
#pragma init_seg(lib)
#endif

/**
 * This macro is defined when no redefinition of \c new is wanted.  This
 * is to ensure that overloading and direct calling of <code>operator
 * new</code> is possible.
 */
#define _DEBUG_NEW_NO_NEW_REDEFINITION
#include "debug_new.h"

/**
 * Structure to store the position information where \c new occurs.
 */
struct new_ptr_list_t
{
    new_ptr_list_t*     next;
#if _DEBUG_NEW_FILENAME_LEN == 0
    const char*         file;
#else
    char                file[_DEBUG_NEW_FILENAME_LEN];
#endif
    int                 line;
    size_t              size;
};

/**
 * Array of pointer lists of a hash value.
 */
static new_ptr_list_t* new_ptr_list[_DEBUG_NEW_HASHTABLESIZE];

/**
 * Array of mutex guards to protect simultaneous access to the pointer
 * lists of a hash value.
 */
static fast_mutex new_ptr_lock[_DEBUG_NEW_HASHTABLESIZE];

/**
 * Total memory allocated in bytes.
 */
static size_t total_mem_alloc = 0;

/**
 * Flag to control whether #check_leaks will be automatically called on
 * program exit.
 */
bool new_autocheck_flag = true;

/**
 * Flag to control whether verbose messages are output.
 */
bool new_verbose_flag = false;

/**
 * Pointer to the output stream.
 */
FILE* new_output_fp = stderr;

/**
 * Searches for the raw pointer given a user pointer.  The term `raw
 * pointer' here refers to the pointer to the pointer to originally
 * <em>malloc</em>'d memory.
 *
 * @param pointer       user pointer to search for
 * @param hash_index    hash index of the user pointer
 * @return              the raw pointer if searching is successful; or
 *                      \c NULL otherwise
 */
static new_ptr_list_t** search_pointer(void* pointer, size_t hash_index)
{
    new_ptr_list_t** raw_ptr = &new_ptr_list[hash_index];
    while (*raw_ptr)
    {
        if ((char*)*raw_ptr + sizeof(new_ptr_list_t) == pointer)
        {
            return raw_ptr;
        }
        raw_ptr = &(*raw_ptr)->next;
    }
    return NULL;
}

/**
 * Frees memory and adjusts pointers relating to a raw pointer.  If the
 * highest bit of \c line (set from a previous <code>new[]</code> call)
 * does not agree with \c array_mode, program will abort with an error
 * message.
 *
 * @param raw_ptr       raw pointer to free
 * @param array_mode    flag indicating whether it is invoked by a
 *                      <code>delete[]</code> call
 */
static void free_pointer(new_ptr_list_t** raw_ptr, bool array_mode)
{
    new_ptr_list_t* ptr = *raw_ptr;
    int array_mode_mismatch = array_mode ^ ((ptr->line & INT_MIN) != 0);
    if (array_mode_mismatch)
    {
        const char* msg;
        if (array_mode)
            msg = "delete[] after new";
        else
            msg = "delete after new[]";
        fprintf(new_output_fp,
                "%s at %p (size %u, allocated from %s:%d)\n",
                msg,
                (char*)ptr + sizeof(new_ptr_list_t),
                ptr->size,
                ptr->file,
                ptr->line & ~INT_MIN);
        _DEBUG_NEW_ERROR_ACTION;
    }
    total_mem_alloc -= ptr->size;
    if (new_verbose_flag)
    {
        fprintf(new_output_fp,
                "delete: freeing  %p (size %u, %u bytes still allocated)\n",
                (char*)ptr + sizeof(new_ptr_list_t),
                ptr->size, total_mem_alloc);
    }
    *raw_ptr = ptr->next;
    free(ptr);
    return;
}

/**
 * Checks for memory leaks.
 *
 * @return  zero if no leakage is found; the number of leaks otherwise
 */
int check_leaks()
{
    int leak_cnt = 0;
    for (int i = 0; i < _DEBUG_NEW_HASHTABLESIZE; ++i)
    {
        fast_mutex_autolock lock(new_ptr_lock[i]);
        new_ptr_list_t* ptr = new_ptr_list[i];
        if (ptr == NULL)
            continue;
        while (ptr)
        {
            fprintf(new_output_fp,
                    "Leaked object at %p (size %u, %s:%d)\n",
                    (char*)ptr + sizeof(new_ptr_list_t),
                    ptr->size,
                    ptr->file,
                    ptr->line & ~INT_MIN);
            ptr = ptr->next;
            ++leak_cnt;
        }
    }
    return leak_cnt;
}

void* operator new(size_t size, const char* file, int line)
{
    assert((line & INT_MIN) == 0);
    size_t s = size + sizeof(new_ptr_list_t);
    new_ptr_list_t* ptr = (new_ptr_list_t*)malloc(s);
    if (ptr == NULL)
    {
        fprintf(new_output_fp,
                "new:  out of memory when allocating %u bytes\n",
                size);
        _DEBUG_NEW_ERROR_ACTION;
    }
    void* pointer = (char*)ptr + sizeof(new_ptr_list_t);
    size_t hash_index = _DEBUG_NEW_HASH(pointer);
#if _DEBUG_NEW_FILENAME_LEN == 0
    ptr->file = file;
#else
    strncpy(ptr->file, file, _DEBUG_NEW_FILENAME_LEN - 1)
            [_DEBUG_NEW_FILENAME_LEN - 1] = '\0';
#endif
    ptr->line = line;
    ptr->size = size;
    new_ptr_lock[hash_index].lock();
    ptr->next = new_ptr_list[hash_index];
    new_ptr_list[hash_index] = ptr;
    new_ptr_lock[hash_index].unlock();
    if (new_verbose_flag)
    {
        fprintf(new_output_fp,
                "new:  allocated  %p (size %u, %s:%d)\n",
                pointer, size, file, line);
    }
    total_mem_alloc += size;
    return pointer;
}

void* operator new[](size_t size, const char* file, int line)
{
    void* pointer = operator new(size, file, line);
    new_ptr_list_t* ptr =
            (new_ptr_list_t*)((char*)pointer - sizeof(new_ptr_list_t));
    assert((ptr->line & INT_MIN) == 0);
    ptr->line |= INT_MIN;   // The highest bit indicates result from new[]
    return pointer;
}

void* operator new(size_t size) throw(std::bad_alloc)
{
    return operator new(size, "<Unknown>", 0);
}

void* operator new[](size_t size) throw(std::bad_alloc)
{
    return operator new[](size, "<Unknown>", 0);
}

void* operator new(size_t size, const std::nothrow_t&) throw()
{
    return operator new(size);
}

void* operator new[](size_t size, const std::nothrow_t&) throw()
{
    return operator new[](size);
}

void operator delete(void* pointer) throw()
{
    if (pointer == NULL)
        return;
    size_t hash_index = _DEBUG_NEW_HASH(pointer);
    fast_mutex_autolock lock(new_ptr_lock[hash_index]);
    new_ptr_list_t** raw_ptr = search_pointer(pointer, hash_index);
    if (raw_ptr == NULL)
    {
        fprintf(new_output_fp, "delete: invalid pointer %p\n", pointer);
        _DEBUG_NEW_ERROR_ACTION;
    }
    free_pointer(raw_ptr, false);
}

void operator delete[](void* pointer) throw()
{
    if (pointer == NULL)
        return;
    size_t hash_index = _DEBUG_NEW_HASH(pointer);
    fast_mutex_autolock lock(new_ptr_lock[hash_index]);
    new_ptr_list_t** raw_ptr = search_pointer(pointer, hash_index);
    if (raw_ptr == NULL)
    {
        fprintf(new_output_fp, "delete[]: invalid pointer %p\n", pointer);
        _DEBUG_NEW_ERROR_ACTION;
    }
    free_pointer(raw_ptr, true);
}

// Some older compilers like Borland C++ Compiler 5.5.1 and Digital Mars
// Compiler 8.29 do not support placement delete operators.
// NO_PLACEMENT_DELETE needs to be defined when using such compilers.
// Also note that in that case memory leakage will occur if an exception
// is thrown in the initialization (constructor) of a dynamically
// created object.
#ifndef NO_PLACEMENT_DELETE
void operator delete(void* pointer, const char* file, int line) throw()
{
    if (new_verbose_flag)
    {
        fprintf(new_output_fp,
                "info: exception thrown on initializing object"
                " at %p (%s:%d)\n",
                pointer, file, line);
    }
    operator delete(pointer);
}

void operator delete[](void* pointer, const char* file, int line) throw()
{
    if (new_verbose_flag)
    {
        fprintf(new_output_fp,
                "info: exception thrown on initializing objects"
                " at %p (%s:%d)\n",
                pointer, file, line);
    }
    operator delete[](pointer);
}

void operator delete(void* pointer, const std::nothrow_t&) throw()
{
    operator delete(pointer, "<Unknown>", 0);
}

void operator delete[](void* pointer, const std::nothrow_t&) throw()
{
    operator delete[](pointer, "<Unknown>", 0);
}
#endif // NO_PLACEMENT_DELETE

int __debug_new_counter::_count = 0;

/**
 * Constructor to increment the count.
 */
__debug_new_counter::__debug_new_counter()
{
    ++_count;
}

/**
 * Destructor to decrement the count.  When the count is zero,
 * #check_leaks will be called.
 */
__debug_new_counter::~__debug_new_counter()
{
    if (--_count == 0 && new_autocheck_flag)
        if (check_leaks())
            new_verbose_flag = true;
}
