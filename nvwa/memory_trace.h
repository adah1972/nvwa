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
 * @file  memory_trace.h
 *
 * Header file for tracing memory with contextual checkpoints.  The
 * current code requires a C++17-compliant compiler.
 *
 * @date  2025-02-02
 */

#ifndef NVWA_MEMORY_TRACE_H
#define NVWA_MEMORY_TRACE_H

#ifdef NVWA_DEBUG_NEW_H
#error "memory_trace is incompatible with debug_new."
#endif

#include <stddef.h>             // size_t
#include <stdio.h>              // FILE
#include <new>                  // std::align_val_t
#include "_nvwa.h"              // NVWA macros
#include "context.h"            // nvwa::context/NVWA_CONTEXT_CHECKPOINT

NVWA_NAMESPACE_BEGIN

int check_leaks();
size_t get_current_mem_alloc();
size_t get_total_mem_alloc_cnt();

extern bool new_autocheck_flag; // default to true: call check_leaks() on exit
extern bool new_verbose_flag;   // default to false: no verbose information
extern FILE* new_output_fp;     // default to stderr: output to console

class memory_trace_counter {
public:
    memory_trace_counter();
    ~memory_trace_counter();
    memory_trace_counter(const memory_trace_counter&) = delete;
    memory_trace_counter& operator=(const memory_trace_counter&) = delete;

private:
    static int _S_count;
};

static memory_trace_counter __memory_trace_count;

NVWA_NAMESPACE_END

#define NVWA_MEMORY_CHECKPOINT NVWA_CONTEXT_CHECKPOINT

void* operator new  (size_t size,
                     const NVWA::context& ctx);
void* operator new[](size_t size,
                     const NVWA::context& ctx);
void* operator new  (size_t size,
                     std::align_val_t align_val,
                     const NVWA::context& ctx);
void* operator new[](size_t size,
                     std::align_val_t align_val,
                     const NVWA::context& ctx);

void operator delete  (void* ptr,
                       const NVWA::context&) noexcept;
void operator delete[](void* ptr,
                       const NVWA::context&) noexcept;
void operator delete  (void* ptr,
                       std::align_val_t align_val,
                       const NVWA::context&) noexcept;
void operator delete[](void* ptr,
                       std::align_val_t align_val,
                       const NVWA::context&) noexcept;

#endif // NVWA_MEMORY_TRACE_H
