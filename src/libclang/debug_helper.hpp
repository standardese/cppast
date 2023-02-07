// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_DEBUG_HELPER_HPP_INCLUDED
#define CPPAST_DEBUG_HELPER_HPP_INCLUDED

#include <string>

#include "raii_wrapper.hpp"

namespace cppast
{
namespace detail
{
    cxstring get_display_name(const CXCursor& cur) noexcept;

    cxstring get_cursor_kind_spelling(const CXCursor& cur) noexcept;

    cxstring get_type_kind_spelling(const CXType& type) noexcept;

    void print_cursor_info(const CXCursor& cur) noexcept;

    void print_type_info(const CXType& type) noexcept;

    void print_tokens(const CXTranslationUnit& tu, const CXFile& file,
                      const CXCursor& cur) noexcept;
} // namespace detail
} // namespace cppast

#endif // CPPAST_DEBUG_HELPER_HPP_INCLUDED
