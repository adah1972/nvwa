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
 * @date  2017-09-12
 */

#include "mmap_reader_base.h"   // nvwa::mmap_reader_base
#include <stdio.h>              // snprintf
#include <stdint.h>             // SIZE_MAX
#include <stdexcept>            // std::runtime_error
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

void throw_runtime_error(const char* reason)
{
    char msg[80];
#if NVWA_UNIX
    snprintf(msg, sizeof msg, "%s failed: %s", reason, strerror(errno));
#else
    char buffer[72];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL,
                   GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buffer,
                   sizeof buffer,
                   NULL);
    int len = snprintf(msg, sizeof msg, "%s failed: %s", reason, buffer);
    if (len >= sizeof msg)
        len = sizeof msg - 1;
    if (msg[len - 1] == '\n')
        msg[len - 1] = '\0';
#endif
    throw std::runtime_error(msg);
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
        throw_runtime_error("open");
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
        throw_runtime_error("CreateFile");
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
        throw_runtime_error("CreateFile");
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
        throw_runtime_error("fstat");
    if (sizeof s.st_size > sizeof(size_t) && s.st_size > SIZE_MAX)
        throw std::runtime_error("file size is too big");
    void* ptr = mmap(_NULLPTR, s.st_size, PROT_READ, MAP_SHARED, _M_fd, 0);
    if (ptr == MAP_FAILED)
        throw_runtime_error("mmap");
    _M_mmap_ptr = static_cast<char*>(ptr);
    _M_size = s.st_size;
#else
    DWORD file_size_high;
    DWORD file_size_low = GetFileSize(_M_file_handle, &file_size_high);
    if (file_size_low == INVALID_FILE_SIZE)
        throw_runtime_error("GetFileSize");
    if (file_size_high == 0)
        _M_size = file_size_low;
    else
    {
#ifdef _WIN64
        _M_size = (SIZE_T(file_size_high) << 32) + file_size_low;
#else
        throw std::runtime_error("file size is too big");
#endif
    }
    _M_map_handle = CreateFileMapping(
            _M_file_handle,
            NULL,
            PAGE_READONLY,
            file_size_high,
            file_size_low,
            NULL);
    if (_M_map_handle == NULL)
        throw_runtime_error("CreateFileMapping");
    _M_mmap_ptr = static_cast<char*>(MapViewOfFile(
            _M_map_handle,
            FILE_MAP_READ,
            0,
            0,
            _M_size));
    if (_M_mmap_ptr == NULL)
        throw_runtime_error("MapViewOfFile");
#endif
}

NVWA_NAMESPACE_END
