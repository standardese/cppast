// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_ASSERT_HPP_INCLUDED
#define CPPAST_ASSERT_HPP_INCLUDED

#include <debug_assert.hpp>

#ifndef CPPAST_ASSERTION_LEVEL
#    define CPPAST_ASSERTION_LEVEL 0
#endif

#ifndef CPPAST_PRECONDITION_LEVEL
#    ifdef NDEBUG
#        define CPPAST_PRECONDITION_LEVEL 0
#    else
#        define CPPAST_PRECONDITION_LEVEL 1
#    endif
#endif

namespace cppast
{
namespace detail
{
    struct assert_handler : debug_assert::set_level<CPPAST_ASSERTION_LEVEL>,
                            debug_assert::default_handler
    {};

    struct precondition_error_handler : debug_assert::set_level<CPPAST_PRECONDITION_LEVEL>,
                                        debug_assert::default_handler
    {};
} // namespace detail
} // namespace cppast

#endif // CPPAST_ASSERT_HPP_INCLUDED
