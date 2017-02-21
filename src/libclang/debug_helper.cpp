// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "debug_helper.hpp"

#include <cstdio>
#include <clang-c/Index.h>

using namespace cppast;

void detail::print_cursor_info(const CXCursor& cur) noexcept
{
    std::printf("[debug] cursor '%s' (%s)\n", cxstring(clang_getCursorDisplayName(cur)).c_str(),
                cxstring(clang_getCursorKindSpelling(cur.kind)).c_str());
}
