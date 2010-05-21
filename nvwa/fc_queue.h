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
 * @file    fc_queue.h
 *
 * Definition of a fixed-capacity queue.
 *
 * @version 1.15, 2010/05/16
 * @author  Wu Yongwei
 *
 */

#ifndef _FC_QUEUE_H
#define _FC_QUEUE_H

#include <assert.h>
#include <stddef.h>
#include <algorithm>
#include <new>

#if defined(BOOST_CONFIG_HPP) && !defined(_FC_QUEUE_NO_BOOST_TYPETRAITS)
# include <boost/type_traits.hpp>
# define __true_type  boost::true_type
# define __false_type boost::false_type
#elif defined(_STLPORT_VERSION)
# include <stl/type_traits.h>
# if !defined(_STLP_HAS_NO_NAMESPACES)
using std::__type_traits;
using std::__true_type;
using std::__false_type;
# endif
#elif defined(__GNUC__) && (__GNUC__ >= 3)
# include <bits/type_traits.h>
#else
# include <type_traits.h>
#endif

/**
 * Class to represent a fixed-capacity queue.
 */
template <class _Tp>
class fc_queue
{
public:
    typedef _Tp                 value_type;
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
     */
    fc_queue() : _M_nodes_array(NULL), _M_new(NULL), _M_front(NULL)
               , _M_size(0), _M_max_size(0) {}

    /**
     * Constructor that creates the queue with a maximum size (capacity).
     *
     * @param max_size  the maximum size allowed
     */
    explicit fc_queue(size_type max_size)
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
        while (_M_front)
        {
            destroy(&_M_front->_M_data);
            _M_front = _M_front->_M_next;
        }
        delete[] _M_nodes_array;
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
    bool uninitialized() const
    {
        return _M_nodes_array == NULL;
    }

    /**
     * Checks whether the queue is empty (containing no items).
     *
     * @return  \c true if it is empty; \c false otherwise
     */
    bool empty() const
    {
        return _M_size == 0;
    }

    /**
     * Checks whether the queue is full (containing the maximum allowed
     * items).
     *
     * @return  \c true if it is full; \c false otherwise
     */
    bool full() const
    {
        return _M_size == _M_max_size;
    }

    /**
     * Gets the maximum number of allowed items in the queue.
     *
     * @return  the maximum number of allowed items in the queue
     */
    size_type capacity() const
    {
        return _M_max_size;
    }

    /**
     * Gets the number of existing items in the queue.
     *
     * @return  the number of existing items in the queue
     */
    size_type size() const
    {
        return _M_size;
    }

    /**
     * Gets the first item in the queue.
     *
     * @return  reference to the first item
     */
    reference front()
    {
        assert(!empty());
        return *(pointer)(&_M_front->_M_data);
    }

    /**
     * Gets the first item in the queue.
     *
     * @return  const reference to the first item
     */
    const_reference front() const
    {
        assert(!empty());
        return *(const_pointer)(&_M_front->_M_data);
    }

    /**
     * Gets the last item in the queue.
     *
     * @return  reference to the last item
     */
    reference back()
    {
        assert(!empty());
        return *(pointer)(&_M_back->_M_data);
    }

    /**
     * Gets the last item in the queue.
     *
     * @return  const reference to the last item
     */
    const_reference back() const
    {
        assert(!empty());
        return *(const_pointer)(&_M_back->_M_data);
    };

    /**
     * Inserts a new item at the end of the queue.  The first item will
     * be discarded if the queue is full.
     *
     * @param value  the item to be inserted
     */
    void push(const value_type& value)
    {
        // Use the temporary (register) variable to eliminate
        // repeatedly loading the same value from memory
        register _Node* new_node = _M_new;
        construct(&new_node->_M_data, value);
        if (full())
            pop();
        if (empty())
            _M_front = _M_back = new_node;
        else
        {
            _M_back->_M_next = new_node;
            _M_back          = new_node;
        }
        _M_new = _M_new->_M_next;
        assert(_M_new != NULL);
        new_node->_M_next = NULL;
        ++_M_size;
    }

    /**
     * Discards the first item in the queue.
     */
    void pop()
    {
        assert(!empty());
        destroy(&_M_front->_M_data);
        _Node* old_front = _M_front;
        _M_front = old_front->_M_next;
        assert(_M_free_nodes_end->_M_next == NULL);
        _M_free_nodes_end->_M_next = old_front;
        _M_free_nodes_end = old_front;
        old_front->_M_next = NULL;
        --_M_size;
    }

    /**
     * Checks whether the queue contains a specific item.
     *
     * @param value  the item to be compared
     * @return       \c true if found; \c false otherwise
     */
    bool contains(const value_type& value) const
    {
        register _Node* node = _M_front;
        while (node)
        {
            if (value == *(const_pointer)(&node->_M_data))
                return true;
            node = node->_M_next;
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
        std::swap(_M_nodes_array,    rhs._M_nodes_array);
        std::swap(_M_free_nodes_end, rhs._M_free_nodes_end);
        std::swap(_M_new,            rhs._M_new);
        std::swap(_M_front,          rhs._M_front);
        std::swap(_M_back,           rhs._M_back);
        std::swap(_M_size,           rhs._M_size);
        std::swap(_M_max_size,       rhs._M_max_size);
    }

protected:
    /** Struct to represent a node in the queue. */
    struct _Node
    {
        _Node*  _M_next;
        char    _M_data[sizeof(_Tp)];
    };
    _Node*      _M_nodes_array;
    _Node*      _M_free_nodes_end;
    _Node*      _M_new;
    _Node*      _M_front;
    _Node*      _M_back;
    size_type   _M_size;
    size_type   _M_max_size;

protected:
    void _M_initialize(size_type max_size);
    void _M_destroy(void* pointer, __true_type)
    {}
    void _M_destroy(void* pointer, __false_type)
    {
        ((_Tp*)pointer)->~_Tp();
    }
    void destroy(void* pointer)
    {
#if defined(BOOST_CONFIG_HPP) && !defined(_FC_QUEUE_NO_BOOST_TYPETRAITS)
        _M_destroy(pointer, boost::has_trivial_destructor<_Tp>());
#else
        _M_destroy(pointer,
                  typename __type_traits<_Tp>::has_trivial_destructor());
#endif
    }
    void construct(void* pointer, const _Tp& value)
    {
        new (pointer) _Tp(value);
    }
};

template <class _Tp>
fc_queue<_Tp>::fc_queue(const fc_queue& rhs)
    : _M_nodes_array(NULL), _M_front(NULL)
{
    fc_queue temp(rhs._M_max_size);
    _Node* node = rhs._M_front;
    while (node)
    {
        temp.push(*(pointer)(&node->_M_data));
        node = node->_M_next;
    }
    assert(temp.size() == rhs.size());
    swap(temp);
}

template <class _Tp>
void fc_queue<_Tp>::_M_initialize(size_type max_size)
{
    size_type i;
    _M_nodes_array = new _Node[max_size + 1];
    _M_new = _M_nodes_array;
    for (i = 0; i < max_size; ++i)
        _M_nodes_array[i]._M_next = _M_nodes_array + i + 1;
    _M_nodes_array[i]._M_next = NULL;
    _M_free_nodes_end = _M_nodes_array + max_size;
    _M_front = NULL;
    _M_back = NULL;
    _M_size = 0;
    _M_max_size = max_size;
}

#endif // _FC_QUEUE_H
