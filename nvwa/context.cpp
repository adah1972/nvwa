// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  context.cpp
 *
 * Implementation of contextual tracing.
 *
 * @date  2025-02-06
 */

#include "context.h"            // context declarations
#include <assert.h>             // assert
#include <stdio.h>              // FILE
#include <string.h>             // strcmp
#include <deque>                // std::deque
#include <exception>            // std::uncaught_exception(s)
#include "_nvwa.h"              // NVWA macros
#include "malloc_allocator.h"   // nvwa::malloc_allocator
#include "trace_stack.h"        // nvwa::trace_stack

namespace {

#if __cplusplus >= 201703L
bool uncaught_exception()
{
    return std::uncaught_exceptions() > 0;
}
#else
using std::uncaught_exception;
#endif

thread_local NVWA::trace_stack<
    NVWA::context,
    std::deque<NVWA::context, NVWA::malloc_allocator<NVWA::context>>>
    context_stack{
        std::deque<NVWA::context, NVWA::malloc_allocator<NVWA::context>>{
            {"<UNKNOWN>", "<UNKNOWN>"}}};

void save_context(const NVWA::context& ctx)
{
    context_stack.push(ctx);
}

void restore_context([[maybe_unused]] const NVWA::context& ctx)
{
    assert(!context_stack.empty() && context_stack.top() == ctx);
    context_stack.pop();
    if (!uncaught_exception()) {
        context_stack.discard_popped();
    }
}

} /* unnamed namespace */

NVWA_NAMESPACE_BEGIN

bool operator==(const context& lhs, const context& rhs)
{
    return strcmp(lhs.file, rhs.file) == 0 &&
           strcmp(lhs.func, rhs.func) == 0;
}

bool operator!=(const context& lhs, const context& rhs)
{
    return !(lhs == rhs);
}

checkpoint::checkpoint(const context& ctx) : ctx_(ctx)
{
    save_context(ctx);
}

checkpoint::~checkpoint()
{
    restore_context(ctx_);
}

const context& get_current_context()
{
    assert(!context_stack.empty());
    return context_stack.top();
}

void print_context(const context& ctx, FILE* fp)
{
    fprintf(fp, "context: %s/%s", ctx.file, ctx.func);
}

void print_exception_contexts(FILE* fp)
{
    auto popped_items = context_stack.get_popped();
    auto it = popped_items.rbegin();
    auto end = popped_items.rend();
    for (int i = 0; it != end; ++i, ++it) {
        fprintf(fp, "%d: %s/%s\n", i, it->file, it->func);
    }
}

NVWA_NAMESPACE_END
