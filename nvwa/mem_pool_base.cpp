// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2010 Wu Yongwei <adah at users dot sourceforge dot net>
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
 *      http://sourceforge.net/projects/nvwa
 *
 */

/**
 * @file    mem_pool_base.cpp
 *
 * Implementation for the memory pool base.
 *
 * @version 1.4, 2010/05/16
 * @author  Wu Yongwei
 *
 */

#if defined(_MEM_POOL_USE_MALLOC)
#include <stdlib.h>
#else
#include <new>
#endif

#include "mem_pool_base.h"

/* Defines macros to abstract system memory routines */
# ifdef _MEM_POOL_USE_MALLOC
#   define _MEM_POOL_ALLOCATE(_Sz)    malloc(_Sz)
#   define _MEM_POOL_DEALLOCATE(_Ptr) free(_Ptr)
# else
#   define _MEM_POOL_ALLOCATE(_Sz)    ::operator new((_Sz), std::nothrow)
#   define _MEM_POOL_DEALLOCATE(_Ptr) ::operator delete(_Ptr)
# endif

/**
 * @fn void mem_pool_base::recycle()
 *
 * Recycles unused memory from memory pools.  It is an interface and
 * needs to be implemented in subclasses.
 */

/**
 * Empty base destructor.
 */
mem_pool_base::~mem_pool_base()
{
}

/**
 * Allocates memory from the run-time system.
 *
 * @param size  size of the memory to allocate in bytes
 * @return      pointer to allocated memory block if successful; or
 *              \c NULL if memory allocation fails
 */
void* mem_pool_base::alloc_sys(size_t size)
{
    return _MEM_POOL_ALLOCATE(size);
}

/**
 * Frees memory and returns it to the run-time system.
 *
 * @param pointer  pointer to the memory block previously allocated
 */
void mem_pool_base::dealloc_sys(void* pointer)
{
    _MEM_POOL_DEALLOCATE(pointer);
}
