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
 * @file    class_level_lock.h
 *
 * In essence Loki ClassLevelLockable re-engineered to use a fast_mutex class
 *
 * @version 1.8, 2004/03/19
 * @author  Wu Yongwei
 *
 */

#ifndef _CLASS_LEVEL_LOCK_H
#define _CLASS_LEVEL_LOCK_H

#include "fast_mutex.h"

# ifdef _NOTHREADS
    /**
     * Helper class for class-level locking.  This is the
     * single-threaded implementation.
     */
    template <class _Host>
    class class_level_lock
    {
    public:
        /** Type that provides locking/unlocking semantics. */
        class lock
        {
        public:
            explicit lock(bool __lock = true) {}
            void acquire() {}
            void release() {}
        };

        typedef _Host volatile_type;
    };
# else
    /**
     * Helper class for class-level locking.  This is the multi-threaded
     * implementation.
     */
    template <class _Host>
    class class_level_lock
    {
        static fast_mutex _S_mtx;

    public:
        class lock;
        friend class lock;

        /** Type that provides locking/unlocking semantics. */
        class lock
        {
            bool _M_locked;

            lock(const lock&);
            lock& operator=(const lock&);
        public:
            // Unlike Loki ClassLevelLockable, one may choose not to
            // acquire the lock on initialization, and manually call
            // acquire() and release() as the case need be.  This is for
            // maximum flexibility.  See memory_pool.h for real usage.
            explicit lock(bool __lock = true) : _M_locked(false)
            {
                if (__lock)
                    acquire();
            }
            ~lock()
            {
                if (_M_locked)
                    release();
            }
            void acquire()
            {
                _S_mtx.lock();
                _M_locked = true;
            }
            void release()
            {
                _S_mtx.unlock();
                _M_locked = false;
            }
        };

        typedef volatile _Host volatile_type;
    };

    template <class _Host>
    fast_mutex class_level_lock<_Host>::_S_mtx;
# endif // _NOTHREADS

#endif // _CLASS_LEVEL_LOCK_H
