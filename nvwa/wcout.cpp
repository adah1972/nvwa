// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*-
// vim:tabstop=4:shiftwidth=4:expandtab:

/*
 * Copyright (C) 2025 Wu Yongwei <wuyongwei at gmail dot com>
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
 * @file  wcout.cpp
 *
 * Implementation of a new \c wcout on Linux.  See comments in
 * stdio_wostream.h for details.
 *
 * @date  2025-03-05
 */

#include "wcout.h"              // nvwa::wcout

#if defined(__linux__) && defined(__GLIBCXX__)

#include <stdlib.h>             // atexit
#include <unistd.h>             // STDOUT_FILENO
#include <exception>            // std::terminate
#include <iostream>             // std::cerr/wcout
#include <mutex>                // std::call_once/once_flag
#include <new>                  // placement new
#include "stdio_wostream.h"     // nvwa::stdio_wostream/stream_flush

NVWA_NAMESPACE_BEGIN

namespace {

std::once_flag init_flag;
stdio_wostream* instance_ptr;
alignas(stdio_wostream) char storage[sizeof(stdio_wostream)];

void destroy_instance()
{
    instance_ptr->~stdio_wostream();
    instance_ptr = nullptr;
}

void create_instance()
{
#ifdef __cpp_exceptions
    instance_ptr = new (storage)
        stdio_wostream(STDOUT_FILENO, stream_flush::on_tty_newline);
#else
    std::error_code ec;
    instance_ptr = new (storage)
        stdio_wostream(STDOUT_FILENO, ec, stream_flush::on_tty_newline);
    if (ec) {
        std::cerr << "ERROR: Fail to initialize nvwa::wcout: "
                  << ec.message() << '\n';
        std::terminate();
    }
#endif

    atexit(destroy_instance);
}

} // unnamed namespace

namespace detail {

wcout_initializer::wcout_initializer()
{
    wcout_t::get_instance();
}

std::wostream& wcout_t::get_instance()
{
    std::call_once(init_flag, create_instance);
    return *instance_ptr;
}

} // namespace detail

/**
 * Name that references the new \c wcout object.
 *
 * It is OK to access this object during static initialization and
 * deinitialization, as long as wcout.h is included.  However, using it
 * before \c main is problematic, as locale setup is probably necessary
 * to make the wide stream useful.
 *
 * \c wcerr ties to \c wcout, but \c std::cin does not.  I.e. you should
 * manually flush \c wcout, if input from console follows.
 *
 * @hideinitializer
 */
std::wostream& wcout = *reinterpret_cast<stdio_wostream*>(storage);

NVWA_NAMESPACE_END

#endif
