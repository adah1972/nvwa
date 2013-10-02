// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2004-2013 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file  fc_queue.h
 *
 * Definition of a fixed-capacity queue.
 *
 * @date  2013-10-02
 */

#ifndef NVWA_FC_QUEUE_H
#define NVWA_FC_QUEUE_H

#include <assert.h>             // assert
#include <stddef.h>             // size_t/NULL
#include <algorithm>            // std::swap
#include <memory>               // std::allocator
#include <new>                  // placement new
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NOEXCEPT
#include "type_traits.h"        // nvwa::is_trivially_destructible

NVWA_NAMESPACE_BEGIN

/**
 * Class to represent a fixed-capacity queue.  This class has an
 * interface close to \c std::queue, but it allows very efficient and
 * lockless one-producer, one-consumer access, as long as the producer
 * does not try to queue an item when the queue is already full.
 */
template <class _Tp, class _Alloc = std::allocator<_Tp> >
class fc_queue
{
public:
    typedef _Tp                 value_type;
    typedef _Alloc              allocator_type;
    typedef size_t              size_type;
    typedef value_type*         pointer;
    typedef const value_type*   const_pointer;
    typedef value_type&         reference;
    typedef const value_type&   const_reference;

    /**
     * Default constructor that creates a null queue.  The queue remains
     * in an uninitialized (capacity not set), though defined, state.
     * The member functions empty(), full(), capacity(), and size() still
     * return meaningful results.
     *
     * @param alloc  the allocator to use
     */
    explicit fc_queue(const allocator_type& alloc = allocator_type())
        : _M_head(NULL), _M_tail(NULL), _M_begin(NULL), _M_end(NULL)
        , _M_alloc(alloc)
    {}

    /**
     * Constructor that creates the queue with a maximum size (capacity).
     *
     * @param max_size  the maximum size allowed
     * @param alloc     the allocator to use
     */
    explicit fc_queue(size_type max_size,
                      const allocator_type& alloc = allocator_type())
        : _M_alloc(alloc)
    {
        _M_initialize(max_size);
    }

    /**
     * Copy-constructor that copies all items from another queue.
     *
     * @param rhs  the queue to copy
     */
    fc_queue(const fc_queue& rhs);

    /**
     * Destructor.  It erases all elements and frees memory.
     */
    ~fc_queue()
    {
        while (_M_head != _M_tail)
        {
            destroy(_M_head);
            _M_head = increment(_M_head);
        }
        if (_M_begin)
            _M_alloc.deallocate(_M_begin, _M_end - _M_begin);
    }

    /**
     * Assignment operator that copies all items from another queue.
     *
     * @param rhs  the queue to copy
     */
    fc_queue& operator=(const fc_queue& rhs)
    {
        fc_queue temp(rhs);
        swap(temp);
        return *this;
    }

    /**
     * Initializes the queue with a maximum size (capacity).  The queue
     * must be default-constructed, and this function must not have been
     * called.
     *
     * @param max_size  the maximum size allowed
     */
    void initialize(size_type max_size)
    {
        assert(uninitialized());
        _M_initialize(max_size);
    }

    /**
     * Checks whether the queue has been initialized.
     *
     * @return  \c true if uninitialized; \c false otherwise
     */
    bool uninitialized() const _NOEXCEPT
    {
        return _M_begin == NULL;
    }

    /**
     * Checks whether the queue is empty (containing no items).
     *
     * @return  \c true if it is empty; \c false otherwise
     */
    bool empty() const _NOEXCEPT
    {
        return _M_head == _M_tail;
    }

    /**
     * Checks whether the queue is full (containing the maximum allowed
     * items).
     *
     * @return  \c true if it is full; \c false otherwise
     */
    bool full() const _NOEXCEPT
    {
        return _M_head == increment(_M_tail);
    }

    /**
     * Gets the maximum number of allowed items in the queue.
     *
     * @return  the maximum number of allowed items in the queue
     */
    size_type capacity() const _NOEXCEPT
    {
        return _M_end - _M_begin - 1;
    }

    /**
     * Gets the number of existing items in the queue.
     *
     * @return  the number of existing items in the queue
     */
    size_type size() const _NOEXCEPT
    {
        ptrdiff_t dist = _M_tail - _M_head;
        if (dist < 0)
            dist += _M_end - _M_begin;
        return dist;
    }

    /**
     * Gets the first item in the queue.
     *
     * @return  reference to the first item
     */
    reference front()
    {
        assert(!empty());
        return *_M_head;
    }

    /**
     * Gets the first item in the queue.
     *
     * @return  const reference to the first item
     */
    const_reference front() const
    {
        assert(!empty());
        return *_M_head;
    }

    /**
     * Gets the last item in the queue.
     *
     * @return  reference to the last item
     */
    reference back()
    {
        assert(!empty());
        return *decrement(_M_tail);
    }

    /**
     * Gets the last item in the queue.
     *
     * @return  const reference to the last item
     */
    const_reference back() const
    {
        assert(!empty());
        return *decrement(_M_tail);
    };

    /**
     * Inserts a new item at the end of the queue.  The first item will
     * be discarded if the queue is full.
     *
     * @param value  the item to be inserted
     */
    void push(const value_type& value)
    {
        construct(_M_tail, value);
        if (full())
            pop();
        _M_tail = increment(_M_tail);
    }

    /**
     * Discards the first item in the queue.
     */
    void pop()
    {
        assert(!empty());
        destroy(_M_head);
        _M_head = increment(_M_head);
    }

    /**
     * Checks whether the queue contains a specific item.
     *
     * @param value  the item to be compared
     * @return       \c true if found; \c false otherwise
     */
    bool contains(const value_type& value) const
    {
        pointer ptr = _M_head;
        while (ptr != _M_tail)
        {
            if (*ptr == value)
                return true;
            ptr = increment(ptr);
        }
        return false;
    }

    /**
     * Exchanges the items of two queues.
     *
     * @param rhs  the queue to exchange
     */
    void swap(fc_queue& rhs)
     {
        std::swap(_M_head,  rhs._M_head);
        std::swap(_M_tail,  rhs._M_tail);
        std::swap(_M_begin, rhs._M_begin);
        std::swap(_M_end,   rhs._M_end);
        std::swap(_M_alloc, rhs._M_alloc);
    }

    /**
     * Gets the allocator of the queue.
     *
     * @return  the allocator of the queue
     */
    allocator_type get_allocator() const
    {
        return _M_alloc;
    }

protected:
    pointer         _M_head;
    pointer         _M_tail;
    pointer         _M_begin;
    pointer         _M_end;
    allocator_type  _M_alloc;

protected:
    pointer increment(pointer ptr) const _NOEXCEPT
    {
        ++ptr;
        if (ptr == _M_end)
            ptr = _M_begin;
        return ptr;
    }
    pointer decrement(pointer ptr) const _NOEXCEPT
    {
        if (ptr == _M_begin)
            ptr = _M_end;
        return --ptr;
    }
    void construct(void* ptr, const _Tp& value)
    {
        new (ptr) _Tp(value);
    }
    void destroy(void* ptr)
    {
        _M_destroy(ptr, is_trivially_destructible<_Tp>());
    }

private:
    void _M_initialize(size_type max_size);
    void _M_destroy(void*, true_type)
    {}
    void _M_destroy(void* ptr, false_type)
    {
        ((_Tp*)ptr)->~_Tp();
    }
};

template <class _Tp, class _Alloc>
fc_queue<_Tp, _Alloc>::fc_queue(const fc_queue& rhs)
    : _M_head(NULL), _M_tail(NULL), _M_begin(NULL)
{
    fc_queue temp(rhs.capacity(), rhs.get_allocator());
    pointer ptr = rhs._M_head;
    while (ptr != rhs._M_tail)
    {
        temp.push(*ptr);
        ptr = rhs.increment(ptr);
    }
    swap(temp);
}

template <class _Tp, class _Alloc>
void fc_queue<_Tp, _Alloc>::_M_initialize(size_type max_size)
{
    _M_begin = _M_alloc.allocate(max_size + 1);
    _M_end = _M_begin + max_size + 1;
    _M_head = _M_tail = _M_begin;
}

template <class _Tp, class _Alloc>
void swap(fc_queue<_Tp, _Alloc>& lhs, fc_queue<_Tp, _Alloc>& rhs)
{
    lhs.swap(rhs);
}

NVWA_NAMESPACE_END

#endif // NVWA_FC_QUEUE_H
