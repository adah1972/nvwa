// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2017 Wu Yongwei <adah at users dot sourceforge dot net>
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
 * @file  mmap_line_reader.cpp
 *
 * Code for mmap_reader_base, common base for mmap-based file readers.
 * It is implemented with the POSIX mmap API.
 *
 * @date  2017-09-10
 */

#include "mmap_reader_base.h"   // nvwa::mmap_reader_base
#include <errno.h>              // errno
#include <fcntl.h>              // open
#include <stdio.h>              // snprintf
#include <string.h>             // strerror
#include <sys/mman.h>           // mmap/munmap
#include <sys/stat.h>           // fstat
#include <unistd.h>             // close
#include <stdexcept>            // std::runtime_error
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++11.h"              // _NULLPTR

namespace {

void throw_runtime_error(const char* reason)
{
    char msg[80];
    snprintf(msg, sizeof msg, "%s failed: %s", reason, strerror(errno));
    throw std::runtime_error(msg);
}

} /* unnamed namespace */

NVWA_NAMESPACE_BEGIN

/**
 * Constructor.
 *
 * @param path       path to the file to open
 * @param delimiter  the delimiter between text `lines' (default to LF)
 * @param strip      enumerator about whether to strip the delimiter
 */
mmap_reader_base::mmap_reader_base(const char* path)
{
    _M_fd = open(path, O_RDONLY);
    if (_M_fd < 0)
        throw_runtime_error("open");
    initialize();
}

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

/** Destructor. */
mmap_reader_base::~mmap_reader_base()
{
    munmap(_M_mmap_ptr, _M_size);
    close(_M_fd);
}

/**
 * Initializes the object.  It gets the file size and mmaps the whole file.
 */
void mmap_reader_base::initialize()
{
    struct stat s;
    if (fstat(_M_fd, &s) < 0)
        throw_runtime_error("fstat");
    void* ptr = mmap(_NULLPTR, s.st_size, PROT_READ, MAP_SHARED, _M_fd, 0);
    if (ptr == MAP_FAILED)
        throw_runtime_error("mmap");
    _M_mmap_ptr = static_cast<char*>(ptr);
    _M_size = s.st_size;
}

NVWA_NAMESPACE_END
