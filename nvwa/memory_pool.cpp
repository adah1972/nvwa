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
 * @file    memory_pool.cpp
 *
 * Non-template and non-inline code for the `static' memory pool
 *
 * @version 2.2, 2004/03/28
 * @author  Wu Yongwei
 *
 */

#if defined(MEM_POOL_USE_MALLOC)
#include <stdlib.h>
#else
#include <new>
#endif

#include "memory_pool.h"

/* Defines macros to abstract system memory routines */
# ifdef MEM_POOL_USE_MALLOC
#   define MEM_POOL_ALLOCATE(_Sz)    malloc(_Sz)
#   define MEM_POOL_DEALLOCATE(_Ptr) free(_Ptr)
# else
#   define MEM_POOL_ALLOCATE(_Sz)    ::operator new((_Sz), std::nothrow)
#   define MEM_POOL_DEALLOCATE(_Ptr) ::operator delete(_Ptr)
# endif

memory_pool_base::~memory_pool_base()
{
}

void* memory_pool_base::alloc_sys(size_t __size)
{
    void* __result = MEM_POOL_ALLOCATE(__size);
    if (!__result)
    {
        memory_pool_set::recycle_memory_pools();
        __result = MEM_POOL_ALLOCATE(__size);
    }
    return __result;
}

void memory_pool_base::dealloc_sys(void* __ptr)
{
    MEM_POOL_DEALLOCATE(__ptr);
}

memory_pool_set::memory_pool_set()
{
    MEM_POOL_DEBUG_MSG(false, "The memory pool set is created");
}

memory_pool_set::~memory_pool_set()
{
    lock __guard;
    while (!_M_memory_pool_set.empty())
    {
        __guard.release();

        // The destructor of a memory_pool will remove itself from
        // the memory_pool_set.
        delete *_M_memory_pool_set.begin();

        __guard.acquire();
    }
    MEM_POOL_DEBUG_MSG(false, "The memory pool set is destroyed");
}

memory_pool_set& memory_pool_set::instance()
{
    lock __guard;    
    static memory_pool_set _S_instance;
    return _S_instance;
}

void memory_pool_set::recycle_memory_pools()
{
    instance().recycle();
}

void memory_pool_set::recycle()
{
    lock __guard;
    MEM_POOL_DEBUG_MSG(false, "Memory pools are being recycled");
    std::set<memory_pool_base*>::iterator __end = _M_memory_pool_set.end();
    for (std::set<memory_pool_base*>::iterator
            __i  = _M_memory_pool_set.begin();
            __i != __end; ++__i)
    {
        (*__i)->recycle();
    }
}
