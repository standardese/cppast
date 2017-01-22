// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_KIND_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_KIND_HPP_INCLUDED

#include <cppast/detail/assert.hpp>

namespace cppast
{
    /// All possible kinds of C++ entities.
    enum class cpp_entity_kind
    {
        file_t,

        namespace_t,
        namespace_alias_t,
        using_directive_t,
        using_declaration_t,

        enum_t,
        enum_value_t,

        count,
    };

    /// \returns Whether or not a given entity kind is a C++ type.
    bool is_type(cpp_entity_kind type) noexcept;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_KIND_HPP_INCLUDED
