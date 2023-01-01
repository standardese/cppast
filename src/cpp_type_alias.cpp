// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_type_alias.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_type_alias::kind() noexcept
{
    return cpp_entity_kind::type_alias_t;
}

std::unique_ptr<cpp_type_alias> cpp_type_alias::build(const cpp_entity_index& idx, cpp_entity_id id,
                                                      std::string               name,
                                                      std::unique_ptr<cpp_type> type,
                                                      bool                      use_c_style)
{
    auto result = build(std::move(name), std::move(type), use_c_style);
    idx.register_forward_declaration(std::move(id), type_safe::cref(*result)); // not a definition
    return result;
}

std::unique_ptr<cpp_type_alias> cpp_type_alias::build(std::string               name,
                                                      std::unique_ptr<cpp_type> type,
                                                      bool                      use_c_style)
{
    return std::unique_ptr<cpp_type_alias>(new cpp_type_alias(std::move(name), std::move(type), use_c_style));
}

cpp_entity_kind cpp_type_alias::do_get_entity_kind() const noexcept
{
    return kind();
}
