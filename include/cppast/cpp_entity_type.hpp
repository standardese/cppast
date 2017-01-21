// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED

#include <cppast/detail/assert.hpp>

namespace cppast
{
    /// All possible types of C++ entities.
    enum class cpp_entity_type
    {
        file_t,

        namespace_t,
        namespace_alias_t,
        using_directive_t,

        count,
    };

    /// \returns Whether or not a given entity type is one derived from [cppast::cpp_scope]().
    bool is_scope(cpp_entity_type type) noexcept;

    /// \exclude
    namespace detail
    {
        template <typename T, typename Org>
        T downcast_entity(Org& org, cpp_entity_type dest_type) noexcept
        {
            DEBUG_ASSERT(dest_type == cpp_entity_type::count || org.type() == dest_type,
                         detail::precondition_error_handler{}, "invalid downcast");
            return static_cast<T>(org);
        }
    } // namespace detail
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED
