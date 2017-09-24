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
 * @file  mmap_line_reader.cpp
 *
 * Code for mmap_reader_base, common base for memory-mapped file readers.
 * It is implemented with POSIX and Win32 APIs.
 *
 * @date  2017-09-24
 */

#include "mmap_reader_base.h"   // nvwa::mmap_reader_base
#include <stdint.h>             // SIZE_MAX
#include <stdexcept>            // std::runtime_error
#include <string>               // std::string
#include <system_error>         // std::system_error
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NULLPTR

#if NVWA_UNIX
#include <errno.h>              // errno
#include <fcntl.h>              // open
#include <string.h>             // strerror
#include <sys/mman.h>           // mmap/munmap
#include <sys/stat.h>           // fstat
#include <unistd.h>             // close
#elif NVWA_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>            // Win32 API
#else
#error "Unrecognized platform"
#endif

namespace {

void throw_system_error(const char* reason)
{
    std::string msg(reason);
    msg += " failed";
#if NVWA_UNIX
# ifdef _GLIBCXX_SYSTEM_ERROR
    // Follow GCC/libstdc++ behaviour
    std::error_code ec(errno, std::generic_category());
# else
    std::error_code ec(errno, std::system_category());
# endif
#else
    std::error_code ec(GetLastError(), std::system_category());
#endif

    throw std::system_error(ec, msg);
}

} /* unnamed namespace */

NVWA_NAMESPACE_BEGIN

/**
 * Constructor.
 *
 * @param path       path to the file to open
 */
mmap_reader_base::mmap_reader_base(const char* path)
{
#if NVWA_UNIX
    _M_fd = open(path, O_RDONLY);
    if (_M_fd < 0)
        throw_system_error("open");
#else // NVWA_UNIX
    _M_file_handle = CreateFileA(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (_M_file_handle == INVALID_HANDLE_VALUE)
        throw_system_error("CreateFile");
#endif // NVWA_UNIX
    initialize();
}

#if NVWA_WINDOWS
/**
 * Constructor.
 *
 * @param path       path to the file to open
 */
mmap_reader_base::mmap_reader_base(const wchar_t* path)
{
    _M_file_handle = CreateFileW(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    if (_M_file_handle == INVALID_HANDLE_VALUE)
        throw_system_error("CreateFile");
    initialize();
}
#endif // NVWA_WINDOWS

#if NVWA_UNIX
/**
 * Constructor.
 *
 * @param fd         a file descriptor
 * @param delimiter  the delimiter between text `lines' (default to LF)
 * @param strip      enumerator about whether to strip the delimiter
 */
mmap_reader_base::mmap_reader_base(int fd)
    : _M_fd(fd)
{
    initialize();
}
#endif

/** Destructor. */
mmap_reader_base::~mmap_reader_base()
{
#if NVWA_UNIX
    munmap(_M_mmap_ptr, _M_size);
    close(_M_fd);
#else
    UnmapViewOfFile(_M_mmap_ptr);
    CloseHandle(_M_map_handle);
    CloseHandle(_M_file_handle);
#endif
}

/**
 * Initializes the object.  It gets the file size and mmaps the whole file.
 */
void mmap_reader_base::initialize()
{
#if NVWA_UNIX
    struct stat s;
    if (fstat(_M_fd, &s) < 0)
        throw_system_error("fstat");
    if (sizeof s.st_size > sizeof(size_t) && s.st_size > SIZE_MAX)
        throw std::runtime_error("file size is too big");
    void* ptr = mmap(_NULLPTR, s.st_size, PROT_READ, MAP_SHARED, _M_fd, 0);
    if (ptr == MAP_FAILED)
        throw_system_error("mmap");
    _M_mmap_ptr = static_cast<char*>(ptr);
    _M_size = s.st_size;
#else
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(_M_file_handle, &file_size))
        throw_system_error("GetFileSizeEx");
#ifdef _WIN64
    _M_size = file_size.QuadPart;
#else
    if (file_size.HighPart == 0)
        _M_size = file_size.LowPart;
    else
        throw std::runtime_error("file size is too big");
#endif
    _M_map_handle = CreateFileMapping(
            _M_file_handle,
            NULL,
            PAGE_READONLY,
            file_size.HighPart,
            file_size.LowPart,
            NULL);
    if (_M_map_handle == NULL)
        throw_system_error("CreateFileMapping");
    _M_mmap_ptr = static_cast<char*>(MapViewOfFile(
            _M_map_handle,
            FILE_MAP_READ,
            0,
            0,
            _M_size));
    if (_M_mmap_ptr == NULL)
        throw_system_error("MapViewOfFile");
#endif
}

NVWA_NAMESPACE_END
