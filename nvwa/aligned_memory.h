// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2022 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  aligned_memory.h
 *
 * Header file for aligned memory allocation/deallocation.
 *
 * @date  2022-01-03
 */

#ifndef NVWA_ALIGNED_MEMORY_H
#define NVWA_ALIGNED_MEMORY_H

#include <stddef.h>             // size_t
#include "_nvwa.h"              // NVWA macros

NVWA_NAMESPACE_BEGIN

void* aligned_malloc(size_t size, size_t alignment);
void aligned_free(void* ptr);

NVWA_NAMESPACE_END

#endif // NVWA_ALIGNED_MEMORY_H
