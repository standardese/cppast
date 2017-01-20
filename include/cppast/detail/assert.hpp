// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_ASSERT_HPP_INCLUDED
#define CPPAST_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

namespace cppast
{
    namespace detail
    {
        struct assert_handler : debug_assert::set_level<1>, debug_assert::default_handler
        {
        };

        struct precondition_error_handler : debug_assert::set_level<1>,
                                            debug_assert::default_handler
        {
        };
    }
} // namespace cppast::detail

#endif // CPPAST_ASSERT_HPP_INCLUDED
