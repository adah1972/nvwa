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
 * @file    memory_pool.h
 *
 * Header file for the `static' memory pool
 *
 * @version 2.4, 2004/03/28
 * @author  Wu Yongwei
 *
 */

#ifndef _MEMORY_POOL_H
#define _MEMORY_POOL_H

#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <stddef.h>
#include <assert.h>
#include "class_level_lock.h"

/* Defines Work-around for Microsoft Visual C++ 6.0 and Borland C++ 5.5.1 */
# if (defined(_MSC_VER) && _MSC_VER < 1300) \
        || (defined(__BORLANDC__) && __BORLANDC__ < 0x600)
#   define __PRIVATE public
# else
#   define __PRIVATE private
# endif

/* Defines the macro for debugging output */
# ifdef MEM_POOL_DEBUG
#   include <iostream>
#   define MEM_POOL_DEBUG_MSG(_Lck, _Msg) \
        { \
            memory_pool_set::lock __guard(_Lck); \
            std::cerr << "memory_pool: " << _Msg << std::endl; \
        }
# else
#   define MEM_POOL_DEBUG_MSG(_Lck, _Msg) \
        ((void)0)
# endif

/**
 * Abstract base class for memory_pool to do the recycling operation.
 */
class memory_pool_base
{
public:
    virtual ~memory_pool_base();
    virtual void recycle() = 0;
    static void* alloc_sys(size_t __size);
    static void dealloc_sys(void* __ptr);

    /** Structure to store the next available memory block. */
    struct _Block_list { _Block_list* _M_next; };
};

/**
 * Singleton class to maintain a set of existing instantiations of
 * memory_pool.
 */
class memory_pool_set : public class_level_lock<memory_pool_set>
{
public:
    static memory_pool_set& instance();
    static void recycle_memory_pools();

    void add(memory_pool_base* __memory_pool_p)
    {
        lock __guard;
        _M_memory_pool_set.insert(__memory_pool_p);
        MEM_POOL_DEBUG_MSG(false, "A memory pool is added");
    }
    void remove(memory_pool_base* __memory_pool_p)
    {
        lock __guard;
        _M_memory_pool_set.erase(__memory_pool_p);
        MEM_POOL_DEBUG_MSG(false, "A memory pool is removed");
    }

__PRIVATE:
    ~memory_pool_set();
private:
    memory_pool_set();
    void recycle();
    std::set<memory_pool_base*> _M_memory_pool_set;

    /* Forbid their use */
    memory_pool_set(const memory_pool_set&);
    const memory_pool_set& operator=(const memory_pool_set&);
};

/**
 * Singleton class template to manage the allocation/deallocation of
 * memory blocks of one specific size.
 *
 * @param _Sz   size of elements in the memory pool
 * @param _Gid  group id of a memory pool: if it is negative, access
 *              to this memory pool will be protected from simultaneous
 *              access; otherwise no protection is given
 */
template <size_t _Sz, int _Gid = -1>
class memory_pool : public memory_pool_base
                  , public class_level_lock<memory_pool<_Sz, _Gid> >
{
    typedef typename class_level_lock<memory_pool<_Sz, _Gid> >::lock lock;
public:
    static memory_pool& instance()
    {
        if (!_S_instance_p)
        {
            create_instance();
        }
        return *_S_instance_p;
    }
    static memory_pool& instance_known()
    {
        assert(_S_instance_p != NULL);
        return *_S_instance_p;
    }
    void* allocate()
    {
        void* __result;
        lock __guard(_Gid < 0);
        if (_S_memory_block_p)
        {
            __result = _S_memory_block_p;
            _S_memory_block_p = _S_memory_block_p->_M_next;
            if (_Gid < 0) __guard.release();
        }
        else
        {
            if (_Gid < 0) __guard.release();
            __result = alloc_sys(align(_Sz));
        }
        return __result;
    }
    void deallocate(void* __ptr)
    {
        assert(__ptr != NULL);
        lock __guard(_Gid < 0);
        _Block_list* __block = reinterpret_cast<_Block_list*>(__ptr);
        __block->_M_next = _S_memory_block_p;
        _S_memory_block_p = __block;
    }
    virtual void recycle();

private:
    memory_pool()
    {
        MEM_POOL_DEBUG_MSG(true, "memory_pool<" << _Sz << ',' 
                                 << _Gid << "> is created");
        try
        {
            memory_pool_set::instance().add(this);
        }
        catch (...)
        {
            MEM_POOL_DEBUG_MSG(true, "Exception in memory_pool_set::add");
            throw;
        }
    }
    ~memory_pool()
    {
#   ifdef _DEBUG
        // Empty the pool to avoid false memory leakage alarms.  This is
        // generally not necessary for release binaries.
        _Block_list* __block = _S_memory_block_p;
        while (__block)
        {
            _Block_list* pBlockNext = __block->_M_next;
            dealloc_sys(__block);
            __block = pBlockNext;
        }
        _S_memory_block_p = NULL;
#   endif
        memory_pool_set::instance().remove(this);
        _S_instance_p = NULL;
        _S_destroyed = true;
        MEM_POOL_DEBUG_MSG(true, "memory_pool<" << _Sz << ','
                                 << _Gid << "> is destroyed");
    }
    static void on_dead_reference()
    {
        throw std::runtime_error("dead reference detected");
    }
    static size_t align(size_t __size)
    {
        return __size >= sizeof(_Block_list) ? __size : sizeof(_Block_list);
    }
    static void create_instance();

    static bool _S_destroyed;
    static memory_pool* __VOLATILE _S_instance_p;
    static memory_pool_base::_Block_list* __VOLATILE _S_memory_block_p;

    /* Forbid their use */
    memory_pool(const memory_pool&);
    const memory_pool& operator=(const memory_pool&);
};

template <size_t _Sz, int _Gid> bool
        memory_pool<_Sz, _Gid>::_S_destroyed = false;
template <size_t _Sz, int _Gid> memory_pool<_Sz, _Gid>* __VOLATILE
        memory_pool<_Sz, _Gid>::_S_instance_p = NULL;
template <size_t _Sz, int _Gid> memory_pool_base::_Block_list* __VOLATILE
        memory_pool<_Sz, _Gid>::_S_memory_block_p = NULL;

template <size_t _Sz, int _Gid>
void memory_pool<_Sz, _Gid>::recycle()
{
    lock __guard(_Gid < 0);
    _Block_list* __block = _S_memory_block_p;
    while (__block)
    {
        if (_Block_list* __temp = __block->_M_next)
        {
            _Block_list* __next = __temp->_M_next;
            __block->_M_next = __next;
            dealloc_sys(__temp);
            __block = __next;
        }
        else
        {
            break;
        }
    }
    MEM_POOL_DEBUG_MSG(true, "memory_pool<" << _Sz << ','
                             << _Gid << "> is recycled");
}

template <size_t _Sz, int _Gid>
void memory_pool<_Sz, _Gid>::create_instance()
{
    if (_S_destroyed)
    {
        on_dead_reference();
    }
    else
    {
        lock __guard(_Gid < 0);
        if (!_S_instance_p)
        {
            memory_pool_set::instance();  // Force its creation
            _S_instance_p = new memory_pool();
        }
    }
}

#define DECLARE_MEMORY_POOL(_Cls) \
public: \
    static void* operator new(size_t __size) \
    { \
        assert(__size == sizeof(_Cls)); \
        void* __ptr; \
        __ptr = memory_pool<sizeof(_Cls)>::instance().allocate(); \
        if (__ptr) \
            return __ptr; \
        else \
            throw std::bad_alloc(); \
    } \
    static void operator delete(void* __ptr) \
    { \
        if (__ptr != NULL) \
            memory_pool<sizeof(_Cls)>::instance().deallocate(__ptr); \
    }

#define DECLARE_MEMORY_POOL__NOTHROW(_Cls) \
public: \
    static void* operator new(size_t __size) throw() \
    { \
        assert(__size == sizeof(_Cls)); \
        return memory_pool<sizeof(_Cls)>::instance_known().allocate(); \
    } \
    static void operator delete(void* __ptr) \
    { \
        if (__ptr != NULL) \
            memory_pool<sizeof(_Cls)>::instance_known().deallocate(__ptr); \
    }

#define DECLARE_MEMORY_POOL_GROUPED(_Cls, _Gid) \
public: \
    static void* operator new(size_t __size) \
    { \
        assert(__size == sizeof(_Cls)); \
        void* __ptr; \
        __ptr = memory_pool<sizeof(_Cls), (_Gid)>::instance().allocate(); \
        if (__ptr) \
            return __ptr; \
        else \
            throw std::bad_alloc(); \
    } \
    static void operator delete(void* __ptr) \
    { \
        if (__ptr != NULL) \
            memory_pool<sizeof(_Cls), (_Gid)>::instance().deallocate(__ptr); \
    }

#define DECLARE_MEMORY_POOL_GROUPED__NOTHROW(_Cls, _Gid) \
public: \
    static void* operator new(size_t __size) throw() \
    { \
        assert(__size == sizeof(_Cls)); \
        return memory_pool<sizeof(_Cls), (_Gid)>:: \
                          instance_known().allocate(); \
    } \
    static void operator delete(void* __ptr) \
    { \
        if (__ptr != NULL) \
            memory_pool<sizeof(_Cls), (_Gid)>:: \
                       instance_known().deallocate(__ptr); \
    }

#define PREPARE_MEMORY_POOL(_Cls) \
    memory_pool<sizeof(_Cls)>::instance()

#define PREPARE_MEMORY_POOL_GROUPED(_Cls, _Gid) \
    memory_pool<sizeof(_Cls), (_Gid)>::instance()

#undef __PRIVATE

#endif // _MEMORY_POOL_H
