// Copyright (C) 2017-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_ASSERT_HPP_INCLUDED
#define CPPAST_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#ifdef CPPAST_ENABLE_ASSERTIONS
#define CPPAST_ASSERTION_LEVEL 1
#else
#define CPPAST_ASSERTION_LEVEL 0
#endif

#ifdef CPPAST_ENABLE_PRECONDITION_CHECKS
#define CPPAST_PRECONDITION_LEVEL 1
#else
#define CPPAST_PRECONDITION_LEVEL 0
#endif

namespace cppast
{
    namespace detail
    {
        struct assert_handler : debug_assert::set_level<CPPAST_ASSERTION_LEVEL>,
                                debug_assert::default_handler
        {
        };

        struct precondition_error_handler : debug_assert::set_level<CPPAST_PRECONDITION_LEVEL>,
                                            debug_assert::default_handler
        {
        };
    }
} // namespace cppast::detail

#endif // CPPAST_ASSERT_HPP_INCLUDED
