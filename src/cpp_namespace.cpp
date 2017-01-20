// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_namespace.hpp>

#include <cppast/cpp_entity_type.hpp>

using namespace cppast;

cpp_entity_type cpp_namespace::do_get_entity_type() const noexcept
{
    return cpp_entity_type::namespace_t;
}
