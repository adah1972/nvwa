// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017-2024 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @date  2024-04-29
 */

#ifndef NVWA_MMAP_READER_BASE_H
#define NVWA_MMAP_READER_BASE_H

#include <stddef.h>             // size_t
#include <system_error>         // std::error_code
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

/** Class to wrap the platform details of an mmapped file. */
class mmap_reader_base {
public:
    mmap_reader_base() = default;
    explicit mmap_reader_base(const char* path);
#if NVWA_WINDOWS
    explicit mmap_reader_base(const wchar_t* path);
#endif
#if NVWA_UNIX
    explicit mmap_reader_base(int fd);
#endif
    mmap_reader_base(const mmap_reader_base&) = delete;
    mmap_reader_base& operator=(const mmap_reader_base&) = delete;
    mmap_reader_base(mmap_reader_base&& rhs) noexcept;
    mmap_reader_base& operator=(mmap_reader_base&& rhs) noexcept;
    ~mmap_reader_base();

    void open(const char* path);
    bool open(const char* path, std::error_code& ec) noexcept;
#if NVWA_WINDOWS
    void open(const wchar_t* path);
    bool open(const wchar_t* path, std::error_code& ec) noexcept;
#endif
#if NVWA_UNIX
    void open(int fd);
    bool open(int fd, std::error_code& ec) noexcept;
#endif
    void close() noexcept;
    bool is_open() const noexcept
    {
        return _M_mmap_ptr != nullptr;
    }

    char* data() const noexcept
    {
        return _M_mmap_ptr;
    }
    size_t size() const noexcept
    {
        return _M_size;
    }

private:
    bool _initialize(std::error_code* ecp);
    bool _open(const char* path, std::error_code* ecp = nullptr);
#if NVWA_WINDOWS
    bool _open(const wchar_t* path, std::error_code* ecp = nullptr);
#endif
#if NVWA_UNIX
    bool _open(int fd, std::error_code* ecp = nullptr);
#endif

    char*         _M_mmap_ptr{};
    size_t        _M_size{};
#if NVWA_UNIX
    int           _M_fd{-1};
#else
    void*         _M_file_handle{};
    void*         _M_map_handle{};
#endif
};

NVWA_NAMESPACE_END

#endif // NVWA_MMAP_READER_BASE_H
