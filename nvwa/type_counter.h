// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2023 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  type_counter.h
 *
 * Utilities for counting the number of types in a certain category, say,
 * counting the number of class template instantiations at the point of use.
 *
 * @date  2023-05-25
 */

#ifndef NVWA_TYPE_COUNTER_H
#define NVWA_TYPE_COUNTER_H

#include "_nvwa.h"              // NVWA macros

NVWA_NAMESPACE_BEGIN

template <typename Category>
class global_counter {
public:
    global_counter() : count_(g_counter) { ++g_counter; }

    int get_count() const { return count_; }
    static int get_total_count() { return g_counter; }

private:
    int count_;
    static int g_counter;
};

template <typename Category>
int global_counter<Category>::g_counter;

template <typename Category, typename T>
struct type_counter {
    static global_counter<Category> counter;
};

template <typename Category, typename T>
global_counter<Category> type_counter<Category, T>::counter;

#define NVWA_COUNT_TYPE(c, t)                                              \
    [[maybe_unused]] static const auto& NVWA_UNIQUE_NAME(type_counter) =   \
        ::NVWA::type_counter<c, t>::counter
#define NVWA_GET_TYPE_NUMBER(c, t)                                         \
    ::NVWA::type_counter<c, t>::counter.get_count()
#define NVWA_GET_TYPE_TOTAL_COUNT(c)                                       \
    ::NVWA::global_counter<c>::get_total_count()

NVWA_NAMESPACE_END

#endif // NVWA_TYPE_COUNTER_H
