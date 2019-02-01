// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_enum.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_enum_value::kind() noexcept
{
    return cpp_entity_kind::enum_value_t;
}

std::unique_ptr<cpp_enum_value> cpp_enum_value::build(const cpp_entity_index& idx, cpp_entity_id id,
                                                      std::string                     name,
                                                      std::unique_ptr<cpp_expression> value)
{
    auto result
        = std::unique_ptr<cpp_enum_value>(new cpp_enum_value(std::move(name), std::move(value)));
    idx.register_definition(std::move(id), type_safe::ref(*result));
    return result;
}

cpp_entity_kind cpp_enum_value::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_enum::kind() noexcept
{
    return cpp_entity_kind::enum_t;
}

cpp_entity_kind cpp_enum::do_get_entity_kind() const noexcept
{
    return kind();
}

type_safe::optional<cpp_scope_name> cpp_enum::do_get_scope_name() const
{
    if (scoped_)
        return type_safe::ref(*this);
    return type_safe::nullopt;
}
