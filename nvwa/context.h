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
 * @file  context.h
 *
 * Header file for contextual tracing.
 *
 * @date  2025-02-08
 */

#ifndef NVWA_CONTEXT_H
#define NVWA_CONTEXT_H

#include <stdio.h>              // FILE
#include "_nvwa.h"              // NVWA macros

NVWA_NAMESPACE_BEGIN

/**
 * Context information to store in checkpoints.
 */
struct context {
    const char* file;  ///< File name
    const char* func;  ///< Function name
};

bool operator==(const context& lhs, const context& rhs);
bool operator!=(const context& lhs, const context& rhs);

/**
 * RAII object to push and pop thread-local contexts automatically.
 */
class checkpoint {
public:
    explicit checkpoint(const context& ctx);
    ~checkpoint();
    checkpoint(const checkpoint&) = delete;
    checkpoint& operator=(const checkpoint&) = delete;

private:
    const context ctx_;
};

const context& get_current_context();
void print_context(const context& ctx, FILE* fp = stdout);
void print_exception_contexts(FILE* fp = stdout);

NVWA_NAMESPACE_END

#ifdef __GNUC__
/** Macro for setting up a checkpoint.  GCC-specific version. */
#define NVWA_CONTEXT_CHECKPOINT()                                \
    NVWA::checkpoint NVWA_UNIQUE_NAME(nvwa_context_checkpoint){  \
        NVWA::context{__FILE__, __PRETTY_FUNCTION__}}
#else
/** Macro for setting up a checkpoint.  Generic version. */
#define NVWA_CONTEXT_CHECKPOINT()                                \
    NVWA::checkpoint NVWA_UNIQUE_NAME(nvwa_context_checkpoint){  \
        NVWA::context{__FILE__, __func__}}
#endif

#endif // NVWA_CONTEXT_H
