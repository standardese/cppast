// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED

namespace cppast
{
    /// All possible types of C++ entities.
    enum class cpp_entity_type
    {
        file_t,

        namespace_t,
    };

    /// \returns Whether or not a given entity type is one derived from [cppast::cpp_scope]().
    bool is_scope(cpp_entity_type type) noexcept;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_TYPE_HPP_INCLUDED
