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
 * Implementation of debug versions of new and delete to check leakage
 *
 * @version 2.3, 2004/04/20
 * @author  Wu Yongwei
 *
 */

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include "fast_mutex.h"

#if !_FAST_MUTEX_CHECK_INITIALIZATION && !defined(_NOTHREADS)
#error "_FAST_MUTEX_CHECK_INITIALIZATION not set: check_leaks may not work"
#endif

// The default behaviour now is to copy the file name, because we found
// that the exit leakage check cannot access the address of the file
// name sometimes (in our case, a core dump will occur when trying to
// access the file name in a shared library after a SIGINT).
#ifndef _DEBUG_NEW_FILENAME_LEN
#define _DEBUG_NEW_FILENAME_LEN  20
#endif
#if _DEBUG_NEW_FILENAME_LEN > 0
#include <string.h>
#endif

#ifndef _DEBUG_NEW_HASHTABLESIZE
#define _DEBUG_NEW_HASHTABLESIZE 16384
#endif

#ifndef _DEBUG_NEW_HASH
#define _DEBUG_NEW_HASH(p) (((unsigned)(p) >> 8) % _DEBUG_NEW_HASHTABLESIZE)
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4073)
#pragma init_seg(lib)
#endif

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
 * Flag to control whether verbose messages are output.
 */
bool new_verbose_flag = false;

/**
 * Flag to control whether #check_leaks will be automatically called on
 * program exit.
 */
bool new_autocheck_flag = true;

/**
 * Pointer to the output stream.
 */
FILE* new_output_fp = stderr;

/**
 * Checks for memory leaks.
 *
 * @return  zero if no leakage is found; the number of leaks otherwise
 */
int check_leaks()
{
    int cLeaked = 0;
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
                    ptr->line);
            ptr = ptr->next;
            ++cLeaked;
        }
    }
    return cLeaked;
}

void* operator new(size_t size, const char* file, int line)
{
    size_t s = size + sizeof(new_ptr_list_t);
    new_ptr_list_t* ptr = (new_ptr_list_t*)malloc(s);
    if (ptr == NULL)
    {
        fprintf(new_output_fp,
                "new:  out of memory when allocating %u bytes\n",
                size);
        abort();
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
    return pointer;
}

void* operator new[](size_t size, const char* file, int line)
{
    return operator new(size, file, line);
}

void* operator new(size_t size) throw(std::bad_alloc)
{
    return operator new(size, "<Unknown>", 0);
}

void* operator new[](size_t size) throw(std::bad_alloc)
{
    return operator new(size);
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
    new_ptr_list_t* ptr = new_ptr_list[hash_index];
    new_ptr_list_t* ptr_last = NULL;
    while (ptr)
    {
        if ((char*)ptr + sizeof(new_ptr_list_t) == pointer)
        {
            if (new_verbose_flag)
            {
                fprintf(new_output_fp,
                        "delete: freeing  %p (size %u)\n",
                        pointer, ptr->size);
            }
            if (ptr_last == NULL)
                new_ptr_list[hash_index] = ptr->next;
            else
                ptr_last->next = ptr->next;
            free(ptr);
            return;
        }
        ptr_last = ptr;
        ptr = ptr->next;
    }
    fprintf(new_output_fp, "delete: invalid pointer %p\n", pointer);
    abort();
}

void operator delete[](void* pointer) throw()
{
    operator delete(pointer);
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
    operator delete(pointer, file, line);
}

void operator delete(void* pointer, const std::nothrow_t&) throw()
{
    operator delete(pointer, "<Unknown>", 0);
}

void operator delete[](void* pointer, const std::nothrow_t&) throw()
{
    operator delete(pointer, std::nothrow);
}
#endif // NO_PLACEMENT_DELETE

int __debug_new_counter::_count = 0;

/**
 *  Constructor to increment the count.
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
