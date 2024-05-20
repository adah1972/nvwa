// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2022-2024 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  malloc_allocator.h
 *
 * An allocator that calls malloc/free instead of operator new/delete.
 * It is necessary in the implementation of operator new/delete in order
 * to avoid dead loops.
 *
 * @date  2024-05-20
 */

#ifndef NVWA_MALLOC_ALLOCATOR_H
#define NVWA_MALLOC_ALLOCATOR_H

#include <stdlib.h>         // size_t/malloc/free
#include <type_traits>      // std::true_type
#include "_nvwa.h"          // NVWA macros

NVWA_NAMESPACE_BEGIN

template <typename T>
struct malloc_allocator {
    typedef T value_type;
    typedef std::true_type is_always_equal;
    typedef std::true_type propagate_on_container_move_assignment;

    malloc_allocator() = default;
    template <typename U>
    malloc_allocator(const malloc_allocator<U>&) {}

    template <typename U>
    struct rebind {
        typedef malloc_allocator<U> other;
    };

    T* allocate(size_t n)
    {
        // NOLINTNEXTLINE(bugprone-sizeof-expression)
        return static_cast<T*>(malloc(n * sizeof(T)));
    }
    void deallocate(T* p, size_t)
    {
        free(p);  // NOLINT false bugprone warnings
    }
};

NVWA_NAMESPACE_END

#endif // NVWA_MALLOC_ALLOCATOR_H
