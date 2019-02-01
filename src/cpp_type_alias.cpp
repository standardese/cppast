// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type_alias.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_type_alias::kind() noexcept
{
    return cpp_entity_kind::type_alias_t;
}

std::unique_ptr<cpp_type_alias> cpp_type_alias::build(const cpp_entity_index& idx, cpp_entity_id id,
                                                      std::string               name,
                                                      std::unique_ptr<cpp_type> type)
{
    auto result = build(std::move(name), std::move(type));
    idx.register_forward_declaration(std::move(id), type_safe::cref(*result)); // not a definition
    return result;
}

std::unique_ptr<cpp_type_alias> cpp_type_alias::build(std::string               name,
                                                      std::unique_ptr<cpp_type> type)
{
    return std::unique_ptr<cpp_type_alias>(new cpp_type_alias(std::move(name), std::move(type)));
}

cpp_entity_kind cpp_type_alias::do_get_entity_kind() const noexcept
{
    return kind();
}
