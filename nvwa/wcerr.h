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
 * @file  wcerr.h
 *
 * Header file for a new \c wcerr on Linux.  See comments in
 * stdio_wostream.h for details.
 *
 * @date  2025-03-05
 */

#ifndef NVWA_WCERR_H
#define NVWA_WCERR_H

#include <iostream>             // std::wcerr
#include "_nvwa.h"              // NVWA_NAMESPACE_*

NVWA_NAMESPACE_BEGIN

#if defined(__linux__) && defined(__GLIBCXX__)

namespace detail {

class wcerr_initializer {
public:
    wcerr_initializer();
};

static wcerr_initializer __wcerr_initializer;

/** Helper class for \c wcerr. */
class wcerr_t {
public:
    /**
     * Gets the instance of \c wcerr.  It ensures the instance is
     * created before use.
     */
    static std::wostream& get_instance();
};

} // namespace detail

extern std::wostream& wcerr;

#else

using std::wcerr;

#endif

NVWA_NAMESPACE_END

#endif // NVWA_WCERR_H
