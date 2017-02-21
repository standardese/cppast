// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DEBUG_HELPER_HPP_INCLUDED
#define CPPAST_DEBUG_HELPER_HPP_INCLUDED

#include "raii_wrapper.hpp"

namespace cppast
{
    namespace detail
    {
        cxstring get_display_name(const CXCursor& cur) noexcept;

        void print_cursor_info(const CXCursor& cur) noexcept;

        void print_tokens(const cxtranslation_unit& tu, const CXFile& file,
                          const CXCursor& cur) noexcept;
    }
} // namespace cppast::detail

#endif // CPPAST_DEBUG_HELPER_HPP_INCLUDED
