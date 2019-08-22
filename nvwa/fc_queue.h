// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2009-2019 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  fc_queue.h
 *
 * Definition of a fixed-capacity queue.
 *
 * @date  2019-08-22
 */

#ifndef NVWA_FC_QUEUE_H
#define NVWA_FC_QUEUE_H

#include <assert.h>             // assert
#include <atomic>               // std::atomic
#include <memory>               // std::addressof/allocator/allocator_traits
#include <new>                  // placement new
#include <type_traits>          // std::is_trivially_destructible
#include <utility>              // std::declval/move/swap
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // C++11 feature detection

#if !(NVWA_CXX11_MODE && HAVE_CXX11_NOEXCEPT && HAVE_CXX11_NULLPTR)
#error "C++11 support not detected"
#endif

#ifndef NVWA_FC_QUEUE_USE_ATOMIC
#define NVWA_FC_QUEUE_USE_ATOMIC 1
#endif

NVWA_NAMESPACE_BEGIN

/**
 * Class to represent a fixed-capacity queue.  This class has an
 * interface close to \c std::queue, but it allows very efficient and
 * lockless one-producer, one-consumer access, as long as the producer
 * does not try to enqueue an element when the queue is already full.
 *
 * @param _Tp     the type of elements in the queue
 * @param _Alloc  allocator to use for memory management
 * @pre           \a _Tp shall be \c CopyConstructible and \c
 *                Destructible, and \a _Alloc shall meet the allocator
 *                requirements (Table 28 in the C++11 spec).
 */
template <class _Tp, class _Alloc = std::allocator<_Tp>>
class fc_queue {
public:
    typedef _Tp                                       value_type;
    typedef _Alloc                                    allocator_type;
    typedef std::allocator_traits<_Alloc>             allocator_traits;
    typedef typename allocator_traits::size_type      size_type;
    typedef typename allocator_traits::pointer        pointer;
    typedef typename allocator_traits::const_pointer  const_pointer;
    typedef value_type&                               reference;
    typedef const value_type&                         const_reference;
    typedef std::atomic<pointer>                      atomic_pointer;

    /**
     * Default-constructor that creates an empty queue.
     *
     * It is not very useful, except as the target of an assignment.
     *
     * @post            The following conditions will hold:
     *                  - <code>empty()</code>
     *                  - <code>full()</code>
     *                  - <code>capacity() == 0</code>
     *                  - <code>size() == 0</code>
     */
    fc_queue() noexcept(noexcept(allocator_type()))
        : _M_head(nullptr)
        , _M_tail(nullptr)
        , _M_begin(nullptr)
        , _M_end(_M_begin + 1)
    {
    }

    /**
     * Constructor that creates the queue with a maximum size (capacity).
     *
     * @param max_size  the maximum size allowed
     * @param alloc     the allocator to use
     * @pre             \a max_size shall be not be zero.
     * @post            Unless memory allocation throws an exception, this
     *                  queue will be constructed with the specified maximum
     *                  size, and the following conditions will hold:
     *                  - <code>empty()</code>
     *                  - <code>! full()</code>
     *                  - <code>capacity() == max_size</code>
     *                  - <code>size() == 0</code>
     *                  - <code>get_allocator() == alloc</code>
     */
    explicit fc_queue(size_type max_size,
                      const allocator_type& alloc = allocator_type())
        : _M_alloc(alloc)
    {
        assert(max_size != 0);
        if (max_size + 1 == 0) {
            throw std::bad_alloc();
        }
        _M_begin = _M_alloc.allocate(max_size + 1);
        _M_end = _M_begin + max_size + 1;
        _M_head = _M_begin;
        _M_tail = _M_begin;
    }

    /**
     * Copy-constructor that copies all elements from another queue.
     *
     * @param rhs  the queue to copy
     * @post       If copy-construction is successful (no exception is
     *             thrown during memory allocation and element copy),
     *             this queue will have the same elements as \a rhs.
     */
    fc_queue(const fc_queue& rhs);

    /**
     * Move-constructor that moves all elements from another queue.
     *
     * @param rhs  the queue to move from
     * @post       If the allocator does not throw on move, this queue
     *             will have the same elements as the original \a rhs.
     */
    fc_queue(fc_queue&& rhs) noexcept(
        noexcept(allocator_type(std::declval<allocator_type&&>())));

    /**
     * Destructor.  It erases all elements and frees memory.
     */
    ~fc_queue() noexcept(noexcept(std::declval<_Tp*>()->~_Tp()))
    {
        pointer ptr = _M_head;
        pointer tail = _M_tail;
        while (ptr != tail) {
            destroy(std::addressof(*ptr));
            self_increment(ptr);
        }
        if (_M_begin) {
            _M_alloc.deallocate(_M_begin, _M_end - _M_begin);
        }
    }

    /**
     * Assignment operator that copies all elements from another queue.
     *
     * @param rhs  the queue to copy
     * @post       If assignment is successful (no exception is thrown
     *             during memory allocation and element copy), this queue
     *             will have the same elements as \a rhs.  Otherwise this
     *             queue is unchanged (strong exception safety is
     *             guaranteed).
     */
    fc_queue& operator=(const fc_queue& rhs)
    {
        fc_queue temp(rhs);
        swap(temp);
        return *this;
    }

    /**
     * Assignment operator that moves all elements from another queue.
     *
     * @param rhs  the queue to move from
     * @post       If assignment is successful (no exception is thrown
     *             during memory allocation and element copy), this queue
     *             will have the same elements as \a rhs.  Otherwise this
     *             queue is unchanged (strong exception safety is
     *             guaranteed).
     */
    fc_queue& operator=(fc_queue&& rhs) noexcept(
        noexcept(allocator_type(std::declval<allocator_type&&>())))
    {
        fc_queue temp(std::move(rhs));
        swap(temp);
        return *this;
    }

    /**
     * Checks whether the queue is empty (containing no elements).
     *
     * @return  \c true if it is empty; \c false otherwise
     */
    bool empty() const noexcept
    {
        return _M_head == _M_tail;
    }

    /**
     * Checks whether the queue is full (containing the maximum allowed
     * elements).
     *
     * @return  \c true if it is full; \c false otherwise
     */
    bool full() const noexcept
    {
        return _M_head == increment(_M_tail);
    }

    /**
     * Gets the maximum number of allowed elements in the queue.
     *
     * @return  the maximum number of allowed elements in the queue
     */
    size_type capacity() const noexcept
    {
        return _M_end - _M_begin - 1;
    }

    /**
     * Gets the number of existing elements in the queue.
     *
     * @return  the number of existing elements in the queue
     */
    size_type size() const noexcept
    {
        typename allocator_traits::difference_type dist = _M_tail - _M_head;
        if (dist < 0) {
            dist += _M_end - _M_begin;
        }
        return dist;
    }

    /**
     * Gets the first element in the queue.
     *
     * @pre     the queue is not empty
     * @return  reference to the first element
     */
    reference front()
    {
        assert(!empty());
        return *_M_head;
    }

    /**
     * Gets the first element in the queue.
     *
     * @pre     the queue is not empty
     * @return  const reference to the first element
     */
    const_reference front() const
    {
        assert(!empty());
        return *_M_head;
    }

    /**
     * Gets the last element in the queue.
     *
     * @pre     the queue is not empty
     * @return  reference to the last element
     */
    reference back()
    {
        assert(!empty());
        return *decrement(_M_tail);
    }

    /**
     * Gets the last element in the queue.
     *
     * @pre     the queue is not empty
     * @return  const reference to the last element
     */
    const_reference back() const
    {
        assert(!empty());
        return *decrement(_M_tail);
    };

    /**
     * Inserts a new element at the end of the queue.  The first element
     * will be discarded if the queue is full.  The behaviour is
     * different from \c write().
     *
     * @param args  arguments to construct a new element
     * @pre         <code>capacity() > 0</code>
     * @post        <code>size() <= capacity() && back() == value</code>,
     *              unless an exception is thrown, in which case this
     *              queue is unchanged (strong exception safety is
     *              guaranteed).
     * @see write
     */
    template <typename... _Targs>
    void push(_Targs&&... args)
    {
        assert(capacity() > 0);
        allocator_traits::construct(_M_alloc, std::addressof(*_M_tail),
                                    std::forward<decltype(args)>(args)...);
        if (full()) {
            pop();
        }
        self_increment(_M_tail);
    }

    /**
     * Discards the first element in the queue.
     *
     * @pre   This queue is not empty.
     * @post  One element is discarded at the front, \c size() is
     *        decremented by one, and \c full() is \c false.
     * @see read
     */
    void pop()
    {
        assert(!empty());
        destroy(std::addressof(*_M_head));
        self_increment(_M_head);
    }

    /**
     * Inserts a new element at the end of the queue when the queue is
     * not full.  This is more performant than separate calls to \c
     * full() and \c push(), especially when C++11 atomics are used.
     *
     * @param args  arguments to construct a new element
     * @return      \c true if the new element is successfully inserted;
     *              \c false if the queue is full
     * @pre         <code>capacity() > 0</code>
     * @post        <code>size() <= capacity() && back() == value</code>,
     *              unless an exception is thrown or the queue is full,
     *              in which case this queue is unchanged (strong
     *              exception safety is guaranteed).
     * @see push
     */
    template <typename... _Targs>
    bool write(_Targs&&... args)
    {
        assert(capacity() > 0);
#if NVWA_FC_QUEUE_USE_ATOMIC
        auto tail = _M_tail.load(std::memory_order_relaxed);
        auto new_tail = increment(tail);
        if (new_tail == _M_head.load(std::memory_order_acquire)) {
            return false;
        }
        allocator_traits::construct(_M_alloc, std::addressof(*tail),
                                    std::forward<decltype(args)>(args)...);
        _M_tail.store(new_tail, std::memory_order_release);
#else
        auto new_tail = increment(_M_tail);
        if (new_tail == _M_head) {
            return false;
        }
        allocator_traits::construct(_M_alloc, std::addressof(*_M_tail),
                                    std::forward<decltype(args)>(args)...);
        _M_tail = new_tail;
#endif
        return true;
    }

    /**
     * Moves the first element in the queue to the destination when the
     * queue is not empty.  This is more performant than separate calls
     * to \c empty(), \c front(), and \c pop(), especially when C++11
     * atomics are used.  Strong exception safety is guaranteed, if move
     * assignment of the value type does not throw.
     *
     * @param[out] dest  destination to store the element
     * @return           \c true if an element is moved out of the
     *                   queue; \c false if the queue is empty
     * @see pop
     */
    bool read(reference dest)
    {
#if NVWA_FC_QUEUE_USE_ATOMIC
        auto head = _M_head.load(std::memory_order_relaxed);
        if (head == _M_tail.load(std::memory_order_acquire)) {
            return false;
        }
        dest = std::move(*head);
        destroy(std::addressof(*head));
        _M_head.store(increment(head), std::memory_order_release);
#else
        if (empty()) {
            return false;
        }
        dest = std::move(*_M_head);
        destroy(std::addressof(*_M_head));
        self_increment(_M_head);
#endif
        return true;
    }

    /**
     * Checks whether the queue contains a specific element.
     *
     * @param value  the value to be compared
     * @pre          \c value_type shall be \c EqualityComparable.
     * @return       \c true if found; \c false otherwise
     */
    bool contains(const value_type& value) const
    {
        pointer ptr = _M_head;
        pointer tail = _M_tail;
        while (ptr != tail) {
            if (*ptr == value) {
                return true;
            }
            self_increment(ptr);
        }
        return false;
    }

    /**
     * Exchanges the elements of two queues.
     *
     * @param rhs  the queue to exchange with
     * @post       If swapping the allocators does not throw, \c *this
     *             will be swapped with \a rhs.  If swapping the
     *             allocators throws with strong exception safety
     *             guarantee, this function will also provide such
     *             guarantee.
     */
    void swap(fc_queue& rhs)
        noexcept(noexcept(std::swap(std::declval<allocator_type&>(),
                                    std::declval<allocator_type&>())))
    {
        using std::swap;
        swap(_M_alloc, rhs._M_alloc);
        swap_pointer(_M_head,  rhs._M_head);
        swap_pointer(_M_tail,  rhs._M_tail);
        swap_pointer(_M_begin, rhs._M_begin);
        swap_pointer(_M_end,   rhs._M_end);
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
#if NVWA_FC_QUEUE_USE_ATOMIC
    atomic_pointer  _M_head;
    atomic_pointer  _M_tail;
#else
    pointer         _M_head;
    pointer         _M_tail;
#endif
    pointer         _M_begin;
    pointer         _M_end;
    allocator_type  _M_alloc;

protected:
    pointer increment(pointer ptr) const noexcept
    {
        ++ptr;
        if (ptr == _M_end) {
            ptr = _M_begin;
        }
        return ptr;
    }
    pointer decrement(pointer ptr) const noexcept
    {
        if (ptr == _M_begin) {
            ptr = _M_end;
        }
        return --ptr;
    }
    void self_increment(pointer& ptr) const noexcept
    {
        ptr = increment(ptr);
    }
    void self_decrement(pointer& ptr) const noexcept
    {
        ptr = decrement(ptr);
    }
    void self_increment(atomic_pointer& ptr) const noexcept
    {
        ptr = increment(ptr.load(std::memory_order_relaxed));
    }
    void self_decrement(atomic_pointer& ptr) const noexcept
    {
        ptr = decrement(ptr.load(std::memory_order_relaxed));
    }
    static void destroy(void* ptr)
        noexcept(noexcept(std::declval<_Tp*>()->~_Tp()))
    {
        _M_destroy(ptr, std::is_trivially_destructible<_Tp>());
    }
    static void swap_pointer(pointer& lhs, pointer& rhs) noexcept
    {
        using std::swap;
        swap(lhs, rhs);
    }
    static void swap_pointer(atomic_pointer& lhs,
                             atomic_pointer& rhs) noexcept
    {
        pointer temp = lhs.load(std::memory_order_relaxed);
        lhs.store(rhs.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
        rhs.store(temp, std::memory_order_relaxed);
    }

private:
    static void _M_destroy(void*, std::true_type)
    {}
    static void _M_destroy(void* ptr, std::false_type)
    {
        ((_Tp*)ptr)->~_Tp();
    }
};

template <class _Tp, class _Alloc>
fc_queue<_Tp, _Alloc>::fc_queue(const fc_queue& rhs)
    : _M_head(nullptr), _M_tail(nullptr), _M_begin(nullptr)
{
    fc_queue temp(rhs.capacity(), rhs.get_allocator());
    pointer ptr = rhs._M_head;
    pointer tail = rhs._M_tail;
    while (ptr != tail) {
        temp.push(*ptr);
        ptr = rhs.increment(ptr);
    }
    swap(temp);
}

template <class _Tp, class _Alloc>
fc_queue<_Tp, _Alloc>::fc_queue(fc_queue&& rhs) noexcept(
    noexcept(allocator_type(std::declval<allocator_type&&>())))
{
    _M_alloc = std::move(rhs._M_alloc);
#if NVWA_FC_QUEUE_USE_ATOMIC
    _M_head.store(rhs._M_head.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
    _M_tail.store(rhs._M_tail.load(std::memory_order_relaxed),
                  std::memory_order_relaxed);
    rhs._M_head.store(nullptr, std::memory_order_relaxed);
    rhs._M_tail.store(nullptr, std::memory_order_relaxed);
#else
    _M_head = rhs._M_head;
    _M_tail = rhs._M_tail;
    rhs._M_head = nullptr;
    rhs._M_tail = nullptr;
#endif
    _M_begin = rhs._M_begin;
    _M_end = rhs._M_end;
    rhs._M_begin = nullptr;
    rhs._M_end = rhs._M_begin + 1;
}

/**
 * Exchanges the elements of two queues.
 *
 * @param lhs  the first queue to exchange
 * @param rhs  the second queue to exchange
 * @post       If swapping the allocators does not throw, \a lhs will be
 *             swapped with \a rhs.  If swapping the allocators throws
 *             with strong exception safety guarantee, this function
 *             will also provide such guarantee.
 */
template <class _Tp, class _Alloc>
void swap(fc_queue<_Tp, _Alloc>& lhs, fc_queue<_Tp, _Alloc>& rhs)
    noexcept(noexcept(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

NVWA_NAMESPACE_END

#endif // NVWA_FC_QUEUE_H
