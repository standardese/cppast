// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "debug_helper.hpp"

#include <cstdio>
#include <mutex>

#include "tokenizer.hpp"

using namespace cppast;

detail::cxstring detail::get_display_name(const CXCursor& cur) noexcept
{
    return cxstring(clang_getCursorDisplayName(cur));
}

detail::cxstring detail::get_cursor_kind_spelling(const CXCursor& cur) noexcept
{
    return cxstring(clang_getCursorKindSpelling(clang_getCursorKind(cur)));
}

detail::cxstring detail::get_type_kind_spelling(const CXType& type) noexcept
{
    return cxstring(clang_getTypeKindSpelling(type.kind));
}

namespace
{
    std::mutex mtx;
}

void detail::print_cursor_info(const CXCursor& cur) noexcept
{
    std::lock_guard<std::mutex> lock(mtx);
    std::printf("[debug] cursor '%s' (%s)\n", get_display_name(cur).c_str(),
                cxstring(clang_getCursorKindSpelling(cur.kind)).c_str());
}

void detail::print_tokens(const CXTranslationUnit& tu, const CXFile& file,
                          const CXCursor& cur) noexcept
{
    std::lock_guard<std::mutex> lock(mtx);
    detail::tokenizer           tokenizer(tu, file, cur);
    for (auto& token : tokenizer)
        std::printf("%s ", token.c_str());
    std::puts("\n");
}
