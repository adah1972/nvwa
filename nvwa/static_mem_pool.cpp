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
 * @file    static_mem_pool.cpp
 *
 * Non-template and non-inline code for the `static' memory pool
 *
 * @version 1.0, 2004/03/29
 * @author  Wu Yongwei
 *
 */

#include "static_mem_pool.h"

static_mem_pool_set::static_mem_pool_set()
{
    STATIC_MEM_POOL_TRACE(false, "The static_mem_pool_set is created");
}

static_mem_pool_set::~static_mem_pool_set()
{
    lock __guard;
    while (!_M_memory_pool_set.empty())
    {
        __guard.release();

        // The destructor of a static_mem_pool will remove itself from
        // the static_mem_pool_set.
        delete *_M_memory_pool_set.begin();

        __guard.acquire();
    }
    STATIC_MEM_POOL_TRACE(false, "The static_mem_pool_set is destroyed");
}

static_mem_pool_set& static_mem_pool_set::instance()
{
    lock __guard;
    static static_mem_pool_set _S_instance;
    return _S_instance;
}

void static_mem_pool_set::recycle()
{
    lock __guard;
    STATIC_MEM_POOL_TRACE(false, "Memory pools are being recycled");
    std::set<mem_pool_base*>::iterator __end = _M_memory_pool_set.end();
    for (std::set<mem_pool_base*>::iterator
            __i  = _M_memory_pool_set.begin();
            __i != __end; ++__i)
    {
        (*__i)->recycle();
    }
}
