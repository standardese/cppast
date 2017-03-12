// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_member_function.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_member_function::kind() noexcept
{
    return cpp_entity_kind::member_function_t;
}

cpp_entity_kind cpp_member_function::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_conversion_op::kind() noexcept
{
    return cpp_entity_kind::conversion_op_t;
}

cpp_entity_kind cpp_conversion_op::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_constructor::do_get_entity_kind() const noexcept
{
    return cpp_entity_kind::constructor_t;
}

cpp_entity_kind cpp_destructor::do_get_entity_kind() const noexcept
{
    return cpp_entity_kind::destructor_t;
}
