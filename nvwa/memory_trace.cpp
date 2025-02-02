// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2022-2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  memory_trace.cpp
 *
 * Implementation of memory tracing facilities.
 *
 * @date  2025-02-02
 */

#include "memory_trace.h"       // memory trace declarations
#include <assert.h>             // assert
#include <stddef.h>             // size_t
#include <stdio.h>              // FILE
#include <stdint.h>             // uint32_t/uintptr_t
#include <stdlib.h>             // abort/malloc/free
#include <new>                  // operator new declarations
#include "_nvwa.h"              // NVWA macros
#include "aligned_memory.h"     // nvwa::aligned_malloc/aligned_free
#include "context.h"            // nvwa::context/get_current_context/...
#include "fast_mutex.h"         // nvwa::fast_mutex/fast_mutex_autolock

#ifndef NVWA_CMT_ERROR_ACTION
#define NVWA_CMT_ERROR_ACTION() abort()
#endif

#define NVWA_CMT_ERROR_MESSAGE(...)                               \
    do {                                                          \
        NVWA::fast_mutex_autolock output_guard{new_output_lock};  \
        fprintf(NVWA::new_output_fp, __VA_ARGS__);                \
    } while (false)

namespace {

NVWA::fast_mutex new_ptr_lock;
NVWA::fast_mutex new_output_lock;

enum is_array_t : uint32_t {
    alloc_is_not_array,
    alloc_is_array
};

} /* unnamed namespace */

NVWA_NAMESPACE_BEGIN

bool new_autocheck_flag = true;
bool new_verbose_flag = false;
FILE* new_output_fp = stderr;
size_t current_mem_alloc = 0;
size_t total_mem_alloc_cnt_accum = 0;

constexpr uint32_t CMT_MAGIC = 0x4D'58'54'43;  // "CTXM";

struct alloc_list_base {
    alloc_list_base* next;  ///< Pointer to the next memory block
    alloc_list_base* prev;  ///< Pointer to the previous memory block
};

struct alloc_list_t : alloc_list_base {
    size_t   size;            ///< Size of the memory block
    context  ctx;             ///< The context
    uint32_t head_size : 31;  ///< Size of this struct, aligned
    uint32_t is_array : 1;    ///< Non-zero iff <em>new[]</em> is used
    uint32_t magic;           ///< Magic number for error detection
};

alloc_list_base alloc_list = {
    &alloc_list,  // head (next)
    &alloc_list,  // tail (prev)
};

constexpr uint32_t align(size_t alignment, size_t s)
{
    return static_cast<uint32_t>((s + alignment - 1) & ~(alignment - 1));
}

alloc_list_t* convert_user_ptr(void* usr_ptr, size_t alignment)
{
    auto offset = reinterpret_cast<uintptr_t>(usr_ptr);
    auto adjusted_ptr = static_cast<char*>(usr_ptr);
    bool is_adjusted = false;

    // Check alignment first
    if (offset % alignment != 0) {
        offset -= sizeof(size_t);
        if (offset % alignment != 0) {
            return nullptr;
        }
        // Likely caused by new[] followed by delete, if we arrive here
        adjusted_ptr = static_cast<char*>(usr_ptr) - sizeof(size_t);
        is_adjusted = true;
    }
    auto ptr = reinterpret_cast<alloc_list_t*>(
        adjusted_ptr - align(alignment, sizeof(alloc_list_t)));
    if (ptr->magic == CMT_MAGIC && (!is_adjusted || ptr->is_array)) {
        return ptr;
    }

    if (!is_adjusted && alignment > sizeof(size_t)) {
        // Again, likely caused by new[] followed by delete, as aligned
        // new[] allocates alignment extra space for the array size.
        ptr = reinterpret_cast<alloc_list_t*>(reinterpret_cast<char*>(ptr) -
                                              alignment);
        is_adjusted = true;
    }
    if (ptr->magic == CMT_MAGIC && (!is_adjusted || ptr->is_array)) {
        return ptr;
    }

    return nullptr;
}

void* alloc_mem(size_t size, const context& ctx, is_array_t is_array,
                size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__)
{
    assert(alignment >= __STDCPP_DEFAULT_NEW_ALIGNMENT__);

    uint32_t aligned_list_node_size = align(alignment, sizeof(alloc_list_t));
    alloc_list_t* ptr = [&] {
        if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
            return static_cast<alloc_list_t*>(
                aligned_malloc(size + aligned_list_node_size, alignment));
        } else {
            return static_cast<alloc_list_t*>(
                malloc(size + aligned_list_node_size));
        }
    }();
    if (ptr == nullptr) {
        return nullptr;
    }

    auto usr_ptr = reinterpret_cast<char*>(ptr) + aligned_list_node_size;
    ptr->ctx = ctx;
    ptr->is_array = is_array;
    ptr->size = size;
    ptr->head_size = aligned_list_node_size;
    ptr->magic = CMT_MAGIC;
    {
        fast_mutex_autolock guard{new_ptr_lock};
        ptr->prev = alloc_list.prev;
        ptr->next = &alloc_list;
        alloc_list.prev->next = ptr;
        alloc_list.prev = ptr;
        current_mem_alloc += size;
        ++total_mem_alloc_cnt_accum;
    }
    if (new_verbose_flag) {
        fast_mutex_autolock guard{new_output_lock};
        fprintf(new_output_fp, "new%s: allocated %p (size %zu, ",
                is_array ? "[]" : "", usr_ptr, size);
        print_context(ptr->ctx, new_output_fp);
        fprintf(new_output_fp, ")\n");
    }
    return usr_ptr;
}

void free_mem(void* usr_ptr, is_array_t is_array,
              size_t alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__)
{
    assert(alignment >= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
    if (usr_ptr == nullptr) {
        return;
    }

    auto ptr = convert_user_ptr(usr_ptr, alignment);
    if (ptr == nullptr) {
        NVWA_CMT_ERROR_MESSAGE("delete%s: invalid pointer %p\n",
                               is_array ? "[]" : "", usr_ptr);
        NVWA_CMT_ERROR_ACTION();
    }
    if (is_array != ptr->is_array) {
        const char* msg =
            is_array ? "delete[] after new" : "delete after new[]";
        NVWA_CMT_ERROR_MESSAGE("%s: pointer %p (size %zu)\n",
                               msg, usr_ptr, ptr->size);
        NVWA_CMT_ERROR_ACTION();
    }
    {
        fast_mutex_autolock guard{new_ptr_lock};
        current_mem_alloc -= ptr->size;
        ptr->magic = 0;
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
    }
    if (new_verbose_flag) {
        fast_mutex_autolock guard{new_output_lock};
        fprintf(new_output_fp,
                "delete%s: freed %p (size %zu, %zu bytes still allocated)\n",
                is_array ? "[]" : "", usr_ptr, ptr->size, current_mem_alloc);
    }

    if (alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__) {
        aligned_free(ptr);
    } else {
        free(ptr);
    }
}

int check_leaks()
{
    int leak_cnt = 0;
    fast_mutex_autolock guard_ptr{new_ptr_lock};
    fast_mutex_autolock guard_output{new_output_lock};
    auto ptr = static_cast<alloc_list_t*>(alloc_list.next);

    while (ptr != &alloc_list) {
        if (ptr->magic != CMT_MAGIC) {
            fprintf(new_output_fp, "error: heap data corrupt near %p\n",
                    &ptr->magic);
            NVWA_CMT_ERROR_ACTION();
        }

        auto usr_ptr = reinterpret_cast<const char*>(ptr) + ptr->head_size;
        fprintf(new_output_fp, "Leaked object at %p (size %zu, ", usr_ptr,
                ptr->size);

        print_context(ptr->ctx, new_output_fp);
        fprintf(new_output_fp, ")\n");

        ptr = static_cast<alloc_list_t*>(ptr->next);
        ++leak_cnt;
    }
    if (leak_cnt) {
        fprintf(new_output_fp, "*** %d leaks found\n", leak_cnt);
    }

    return leak_cnt;
}

size_t get_current_mem_alloc()
{
    return current_mem_alloc;
}

size_t get_total_mem_alloc_cnt()
{
    return total_mem_alloc_cnt_accum;
}

int memory_trace_counter::_S_count = 0;

memory_trace_counter::memory_trace_counter()
{
    ++_S_count;
}

memory_trace_counter::~memory_trace_counter()
{
    if (--_S_count == 0 && new_autocheck_flag) {
        if (check_leaks()) {
            new_verbose_flag = true;
        }
    }
}

NVWA_NAMESPACE_END

void* operator new(size_t size)
{
    return operator new(size, NVWA::get_current_context());
}

void* operator new[](size_t size)
{
    return operator new[](size, NVWA::get_current_context());
}

void* operator new(size_t size, const std::nothrow_t&) noexcept
{
    return alloc_mem(size, NVWA::get_current_context(), alloc_is_not_array);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept
{
    return alloc_mem(size, NVWA::get_current_context(), alloc_is_array);
}

void* operator new(size_t size, std::align_val_t align_val)
{
    return operator new(size, align_val, NVWA::get_current_context());
}

void* operator new[](size_t size, std::align_val_t align_val)
{
    return operator new[](size, align_val, NVWA::get_current_context());
}

void* operator new(size_t size, std::align_val_t align_val,
                   const std::nothrow_t&) noexcept
{
    return alloc_mem(size, NVWA::get_current_context(), alloc_is_not_array,
                     size_t(align_val));
}

void* operator new[](size_t size, std::align_val_t align_val,
                     const std::nothrow_t&) noexcept
{
    return alloc_mem(size, NVWA::get_current_context(), alloc_is_array,
                     size_t(align_val));
}

void* operator new(size_t size, const NVWA::context& ctx)
{
    void* ptr = alloc_mem(size, ctx, alloc_is_not_array);
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new[](size_t size, const NVWA::context& ctx)
{
    void* ptr = alloc_mem(size, ctx, alloc_is_array);
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new(size_t size, std::align_val_t align_val,
                   const NVWA::context& ctx)
{
    void* ptr = alloc_mem(size, ctx, alloc_is_not_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void* operator new[](size_t size, std::align_val_t align_val,
                     const NVWA::context& ctx)
{
    void* ptr = alloc_mem(size, ctx, alloc_is_array, size_t(align_val));
    if (ptr) {
        return ptr;
    } else {
        throw std::bad_alloc();
    }
}

void operator delete(void* ptr) noexcept
{
    NVWA::free_mem(ptr, alloc_is_not_array);
}

void operator delete[](void* ptr) noexcept
{
    NVWA::free_mem(ptr, alloc_is_array);
}

void operator delete(void* ptr, size_t) noexcept
{
    NVWA::free_mem(ptr, alloc_is_not_array);
}

void operator delete[](void* ptr, size_t) noexcept
{
    NVWA::free_mem(ptr, alloc_is_array);
}

void operator delete(void* ptr, std::align_val_t align_val) noexcept
{
    NVWA::free_mem(ptr, alloc_is_not_array, size_t(align_val));
}

void operator delete[](void* ptr, std::align_val_t align_val) noexcept
{
    NVWA::free_mem(ptr, alloc_is_array, size_t(align_val));
}

void operator delete(void* ptr, const NVWA::context&) noexcept
{
    operator delete(ptr);
}

void operator delete[](void* ptr, const NVWA::context&) noexcept
{
    operator delete[](ptr);
}

void operator delete(void* ptr, std::align_val_t align_val,
                     const NVWA::context&) noexcept
{
    operator delete(ptr, align_val);
}

void operator delete[](void* ptr, std::align_val_t align_val,
                       const NVWA::context&) noexcept
{
    operator delete[](ptr, align_val);
}
