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
        void print_cursor_info(const CXCursor& cur) noexcept;
    }
} // namespace cppast::detail

#endif // CPPAST_DEBUG_HELPER_HPP_INCLUDED
