// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2022-2023 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  aligned_memory.cpp
 *
 * Implementation of aligned memory allocation/deallocation.
 *
 * @date  2023-09-20
 */

#include "aligned_memory.h"     // aligned memory declarations
#include <stdlib.h>             // aligned_alloc/free/posix_memalign
#include "_nvwa.h"              // NVWA macros

#if NVWA_WIN32
#include <malloc.h>             // _aligned_malloc/_aligned_free
#endif

NVWA_NAMESPACE_BEGIN

/**
 * Allocates uninitialized storage with specified alignment.  It works
 * with Windows, Unix, or a C++17-conformant environment.
 *
 * @param size       number of bytes to allocate
 * @param alignment  value of alignment
 * @return           non-null pointer if successful; \c nullptr otherwise
 */
void* aligned_malloc(size_t size, [[maybe_unused]] size_t alignment)
{
#if NVWA_WIN32
    return _aligned_malloc(size, alignment);
#elif NVWA_UNIX
    void* memptr{};
    int result = posix_memalign(&memptr, alignment, size);
    if (result == 0) {
        return memptr;
    } else {
        return nullptr;
    }
#else
    // Use C++17 aligned_alloc otherwise
    return aligned_alloc(alignment, size);
#endif
}

/**
 * Deallocates memory previously allocated with nvwa#aligned_malloc.
 *
 * @param ptr   the pointer pointing to the memory to free
 */
void aligned_free(void* ptr)
{
#if NVWA_WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

NVWA_NAMESPACE_END
