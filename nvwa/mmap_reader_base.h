// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  mmap_reader_base.h
 *
 * Header file for mmap_reader_base, common base for mmap-based file
 * readers.  It currently supports POSIX and Win32.
 *
 * @date  2017-09-12
 */

#ifndef NVWA_MMAP_READER_BASE_H
#define NVWA_MMAP_READER_BASE_H

#include <stddef.h>             // ptrdiff_t/size_t
#include <iterator>             // std::input_iterator_tag
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _DELETED

NVWA_NAMESPACE_BEGIN

class mmap_reader_base
{
public:
    typedef ptrdiff_t difference_type;
    typedef size_t    size_type;

    explicit mmap_reader_base(const char* path);
#if NVWA_WINDOWS
    explicit mmap_reader_base(const wchar_t* path);
#endif
#if NVWA_UNIX
    explicit mmap_reader_base(int fd);
#endif
    ~mmap_reader_base();

protected:
    char*         _M_mmap_ptr;
    size_type     _M_size;
#if NVWA_UNIX
    int           _M_fd;
#else
    void*         _M_file_handle;
    void*         _M_map_handle;
#endif

private:
    mmap_reader_base(const mmap_reader_base&) _DELETED;
    mmap_reader_base& operator=(const mmap_reader_base&) _DELETED;

    void initialize();
};

NVWA_NAMESPACE_END

#endif // NVWA_MMAP_READER_BASE_H
