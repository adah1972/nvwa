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
 * @file  stdio_wostream.h
 *
 * Header file for creating `std::wcout`-like objects.  This addresses
 * the limitation where \c std::cout and \c std::wcout (or \c std::cerr
 * and \c std::wcerr) conflict on platforms with strict C stream
 * orientation rules.
 *
 * The C standard explicitly prohibits mixing narrow and wide operations
 * (C11 §7.21.2 Streams):
 *
 * > Byte input/output functions shall not be applied to a wide-oriented
 * > stream, and wide character input/output functions shall not be
 * > applied to a byte-oriented stream.
 *
 * This workaround creates a separate C++ stream using a duplicated file
 * descriptor, isolating narrow and wide streams.  It is tailored for
 * libstdc++ (GCC's standard library), which exhibits this
 * standard-compliant but problematic behaviour on Linux.  Windows and
 * macOS do not seem to have this problem, nor does libc++ on Linux.
 *
 * @date  2025-03-05
 */

#ifndef NVWA_STDIO_WOSTREAM_H
#define NVWA_STDIO_WOSTREAM_H

#if __has_include(<bits/c++config.h>)
#include <bits/c++config.h>
#endif

#if defined(__GLIBCXX__)

#include <errno.h>              // errno
#include <ext/stdio_sync_filebuf.h>  // __gnu_cxx::stdio_sync_filebuf
#include <stdio.h>              // FILE/fclose/fdopen/fileno
#include <unistd.h>             // close/dup/isatty
#include <memory>               // std::unique_ptr/make_unique
#include <ostream>              // std::basic_ostream
#include <system_error>         // std::error_code/system_error/...
#include "_nvwa.h"              // NVWA_NAMESPACE_*
#include "c++_features.h"       // _FALLTHROUGH

NVWA_NAMESPACE_BEGIN

namespace detail {

class file_closer {
public:
    void operator()(FILE* fp) const
    {
        fclose(fp);
    }
};

} // namespace detail

/**
 * Definition of stream flush modes.  Actually, \c platform and
 * \c on_tty_newline are identical on Linux, but let us be explicit
 * here.
 */
enum class stream_flush {
    platform,        ///< Platform default (do not call \c setvbuf)
    manual,          ///< Manual (no automatic flush)
    on_tty_newline,  ///< Flush on newline, but only when output is TTY
    on_newline,      ///< Flush on newline
    always           ///< Always flush (unbuffered)
};

/**
 * A simple ostream template using Unix file I/O.  It duplicates the
 * provided file descriptor to avoid conflicts with existing narrow/wide
 * stream orientations.
 */
template <typename _CharT, typename _Traits = std::char_traits<_CharT>>
class basic_stdio_ostream : public std::basic_ostream<_CharT, _Traits> {
public:
#ifdef __cpp_exceptions
    /**
     * Constructs a basic_stdio_ostream, explicitly allowing exceptions.
     *
     * @param fd    file descriptor to use (writable, e.g. \c STDOUT_FILENO)
     * @param mode  stream flush mode
     * @throw std::system_error  I/O operations fail
     */
    explicit
    basic_stdio_ostream(int fd, stream_flush mode = stream_flush::platform)
    {
        std::error_code ec;
        initialize(fd, mode, ec);
        if (ec) {
            throw std::system_error(ec);
        }
    }
#endif

    /**
     * Constructs a basic_stdio_ostream, reporting errors via \a ec.
     * Notice: This does not mean it cannot throw any exceptions, and
     * \c std::bad_alloc is at least theoretically possible.
     *
     * @param fd    file descriptor to use (writable, e.g. \c STDOUT_FILENO)
     * @param ec    reference to the error code
     * @param mode  stream flush mode
     */
    basic_stdio_ostream(int fd, std::error_code& ec,
                        stream_flush mode = stream_flush::platform)
    {
        initialize(fd, mode, ec);
    }

private:
    void initialize(int fd, stream_flush mode, std::error_code& ec)
    {
        // Duplicate the file descriptor
        int new_fd = dup(fd);
        if (new_fd == -1) {
            ec = std::error_code(errno, std::system_category());
            return;
        }

        // Create a FILE* stream from the duplicated file descriptor
        FILE* fp = fdopen(new_fd, "w");
        if (!fp) {
            ec = std::error_code(errno, std::system_category());
            close(new_fd);
            return;
        }

        // Save FILE* for auto-closing
        _M_fp.reset(fp);

        // Create a stream buffer that owns the FILE*
        _M_buf = std::make_unique<__gnu_cxx::stdio_sync_filebuf<_CharT>>(fp);

        // Set up buffering
        if (set_buffering_mode(fp, mode) != 0) {
            ec = std::error_code(errno, std::system_category());
            return;
        }

        // Use the new buffer
        this->rdbuf(_M_buf.get());
    }

    int set_buffering_mode(FILE* fp, stream_flush mode) {
        switch (mode) {
        case stream_flush::platform:
            return 0;
        case stream_flush::manual:
            return setvbuf(fp, nullptr, _IOFBF, BUFSIZ);  // Fully buffered
        case stream_flush::on_tty_newline:
            if (!isatty(fileno(fp))) {
                return setvbuf(fp, nullptr, _IOFBF, BUFSIZ);  // Fully buffered
            }
            _FALLTHROUGH;
        case stream_flush::on_newline:
            return setvbuf(fp, nullptr, _IOLBF, 0);  // Line buffered
        case stream_flush::always:
            return setvbuf(fp, nullptr, _IONBF, 0);  // Unbuffered
        }
        return 0;
    }

    std::unique_ptr<FILE, detail::file_closer>             _M_fp;
    std::unique_ptr<__gnu_cxx::stdio_sync_filebuf<_CharT>> _M_buf;
};

/** The new wostream type for wcout and wcerr. */
typedef basic_stdio_ostream<wchar_t> stdio_wostream;

NVWA_NAMESPACE_END

#endif // defined(__GLIBCXX__)

#endif // NVWA_STDIO_WOSTREAM_H
