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
 * @file  mmap_reader_base.cpp
 *
 * Code for mmap_reader_base, common base for memory-mapped file readers.
 * It is implemented with POSIX and Win32 APIs.
 *
 * @date  2024-05-20
 */

#include "mmap_reader_base.h"   // nvwa::mmap_reader_base
#include <stddef.h>             // size_t
#include <stdint.h>             // SIZE_MAX
#include <string>               // std::string
#include <system_error>         // std::errc/error_code/system_error
#include <utility>              // std::exchange
#include "_nvwa.h"              // NVWA_NAMESPACE_*

#if NVWA_UNIX
#include <errno.h>              // errno
#include <fcntl.h>              // open
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

std::error_code get_last_error_code()
{
#if NVWA_UNIX
    return std::error_code{errno, std::system_category()};
#else
    return std::error_code{static_cast<int>(GetLastError()),
                           std::system_category()};
#endif
}

void indicate_error(std::error_code* ecp, const std::error_code& ec)
{
    if (!ecp) {
        throw std::system_error(ec);
    }
    *ecp = ec;
}

void indicate_last_op_failure(std::error_code* ecp, const char* op)
{
    if (!ecp) {
        std::string msg(op);
        msg += " failed";
        throw std::system_error(get_last_error_code(), msg);
    }
    *ecp = get_last_error_code();
}

} /* unnamed namespace */

NVWA_NAMESPACE_BEGIN

/**
 * Constructor.
 *
 * @param path          path to the file to open
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
mmap_reader_base::mmap_reader_base(const char* path)
{
    _open(path);
}

/**
 * Opens a file.
 *
 * @param path          path to the file to open
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
void mmap_reader_base::open(const char* path)
{
    _open(path);
}

/**
 * Opens a file.
 *
 * @param path          path to the file to open
 * @param ec            reference to the error_code
 * @return              \c true if successful (\a ec is cleared); \c false
 *                      otherwise (\a ec is updated)
 */
// NOLINTNEXTLINE(bugprone-exception-escape)
bool mmap_reader_base::open(const char* path, std::error_code& ec) noexcept
{
    return _open(path, &ec);
}

bool mmap_reader_base::_open(const char* path, std::error_code* ecp)
{
    close();
#if NVWA_UNIX
    _M_fd = ::open(path, O_RDONLY);
    if (_M_fd < 0) {
        indicate_last_op_failure(ecp, "open");
        return false;
    }
#else // NVWA_UNIX
    _M_file_handle = CreateFileA(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    if (_M_file_handle == INVALID_HANDLE_VALUE) {
        indicate_last_op_failure(ecp, "CreateFile");
        return false;
    }
#endif // NVWA_UNIX
    return _initialize(ecp);
}

#if NVWA_WINDOWS
/**
 * Constructor.
 *
 * @param path          path to the file to open
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
mmap_reader_base::mmap_reader_base(const wchar_t* path)
{
    _open(path);
}

/**
 * Opens a file.
 *
 * @param path          path to the file to open
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
void mmap_reader_base::open(const wchar_t* path)
{
    _open(path);
}

/**
 * Opens a file.
 *
 * @param path          path to the file to open
 * @param ec            reference to the error_code
 * @return              \c true if successful (\a ec is cleared); \c false
 *                      otherwise (\a ec is updated)
 */
// NOLINTNEXTLINE(bugprone-exception-escape)
bool mmap_reader_base::open(const wchar_t* path, std::error_code& ec) noexcept
{
    return _open(path, &ec);
}

bool mmap_reader_base::_open(const wchar_t* path, std::error_code* ecp)
{
    close();
    _M_file_handle = CreateFileW(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    if (_M_file_handle == INVALID_HANDLE_VALUE) {
        indicate_last_op_failure(ecp, "CreateFile");
        return false;
    }
    return _initialize(ecp);
}
#endif // NVWA_WINDOWS

#if NVWA_UNIX
/**
 * Constructor.
 *
 * @param fd            a file descriptor
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
mmap_reader_base::mmap_reader_base(int fd)
{
    _open(fd);
}

/**
 * Constructor.
 *
 * @param fd            a file descriptor
 * @throw system_error  an error occurred when calling a system function, or
 *                      when the file size is too big
 */
void mmap_reader_base::open(int fd)
{
    _open(fd);
}

/**
 * Opens a file.
 *
 * @param fd            a file descriptor
 * @param ec            reference to the error_code
 * @return              \c true if successful (\a ec is cleared); \c false
 *                      otherwise (\a ec is updated)
 */
// NOLINTNEXTLINE(bugprone-exception-escape)
bool mmap_reader_base::open(int fd, std::error_code& ec) noexcept
{
    return _open(fd, &ec);
}

bool mmap_reader_base::_open(int fd, std::error_code* ecp)
{
    close();
    _M_fd = fd;
    return _initialize(ecp);
}
#endif

/** Move constructor. */
mmap_reader_base::mmap_reader_base(mmap_reader_base&& rhs) noexcept
    : _M_mmap_ptr(std::exchange(rhs._M_mmap_ptr, nullptr)),
      _M_size(std::exchange(rhs._M_size, 0)),
#if NVWA_UNIX
      _M_fd(rhs._M_fd)
#else
      _M_file_handle(rhs._M_file_handle),
      _M_map_handle(rhs._M_map_handle)
#endif
{
}

/** Move assignment operator. */
mmap_reader_base& mmap_reader_base::operator=(mmap_reader_base&& rhs) noexcept
{
    if (this != &rhs) {
        _M_mmap_ptr = std::exchange(rhs._M_mmap_ptr, nullptr);
        _M_size = std::exchange(rhs._M_size, 0);
#if NVWA_UNIX
        _M_fd = rhs._M_fd;
#else
        _M_file_handle = rhs._M_file_handle;
        _M_map_handle = rhs._M_map_handle;
#endif
    }
    return *this;
}

/** Destructor. */
mmap_reader_base::~mmap_reader_base()
{
    close();
}

void mmap_reader_base::close() noexcept
{
    if (_M_mmap_ptr) {
#if NVWA_UNIX
        munmap(_M_mmap_ptr, _M_size);
        ::close(_M_fd);
#else
        UnmapViewOfFile(_M_mmap_ptr);
        CloseHandle(_M_map_handle);
        CloseHandle(_M_file_handle);
#endif
        _M_mmap_ptr = nullptr;
    }
}

/**
 * Initializes the object.  It gets the file size and mmaps the whole file.
 * This function can throw only if \a ecp is null.
 *
 * @param ecp           pointer to the error_code
 */
bool mmap_reader_base::_initialize(std::error_code* ecp)
{
#if NVWA_UNIX
    struct stat s;  // NOLINT(cppcoreguidelines-pro-type-member-init)
    if (fstat(_M_fd, &s) < 0) {
        indicate_last_op_failure(ecp, "fstat");
        return false;
    }
    if (sizeof s.st_size > sizeof(size_t) && s.st_size > SIZE_MAX) {
        indicate_error(ecp, make_error_code(std::errc::file_too_large));
        return false;
    }
    void* ptr = mmap(nullptr, s.st_size, PROT_READ, MAP_SHARED, _M_fd, 0);
    if (ptr == MAP_FAILED) {
        indicate_last_op_failure(ecp, "mmap");
        return false;
    }
    _M_mmap_ptr = static_cast<char*>(ptr);
    _M_size = s.st_size;
#else
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(_M_file_handle, &file_size)) {
        indicate_last_op_failure(ecp, "GetFileSizeEx");
        return false;
    }
#ifdef _WIN64
    _M_size = file_size.QuadPart;
#else
    if (file_size.HighPart == 0) {
        _M_size = file_size.LowPart;
    } else {
        indicate_error(ecp, make_error_code(std::errc::file_too_large));
        return false;
    }
#endif
    _M_map_handle = CreateFileMapping(
            _M_file_handle,
            nullptr,
            PAGE_READONLY,
            file_size.HighPart,
            file_size.LowPart,
            nullptr);
    if (_M_map_handle == nullptr) {
        indicate_last_op_failure(ecp, "CreateFileMapping");
        return false;
    }
    _M_mmap_ptr = static_cast<char*>(MapViewOfFile(
            _M_map_handle,
            FILE_MAP_READ,
            0,
            0,
            _M_size));
    if (_M_mmap_ptr == nullptr) {
        indicate_last_op_failure(ecp, "MapViewOfFile");
        return false;
    }
#endif
    if (ecp) {
        ecp->clear();
    }
    return true;
}

NVWA_NAMESPACE_END
