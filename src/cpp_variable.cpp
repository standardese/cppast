// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_variable.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_variable::kind() noexcept
{
    return cpp_entity_kind::variable_t;
}

std::unique_ptr<cpp_variable> cpp_variable::build(
    const cpp_entity_index& idx, cpp_entity_id id, std::string name, std::unique_ptr<cpp_type> type,
    std::unique_ptr<cpp_expression> def, cpp_storage_class_specifiers spec, bool is_constexpr,
    type_safe::optional<cpp_entity_ref> semantic_parent)
{
    auto result = std::unique_ptr<cpp_variable>(
        new cpp_variable(std::move(name), std::move(type), std::move(def), spec, is_constexpr));
    result->set_semantic_parent(std::move(semantic_parent));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

std::unique_ptr<cpp_variable> cpp_variable::build_declaration(
    cpp_entity_id definition_id, std::string name, std::unique_ptr<cpp_type> type,
    cpp_storage_class_specifiers spec, bool is_constexpr,
    type_safe::optional<cpp_entity_ref> semantic_parent)
{
    auto result = std::unique_ptr<cpp_variable>(
        new cpp_variable(std::move(name), std::move(type), nullptr, spec, is_constexpr));
    result->set_semantic_parent(std::move(semantic_parent));
    result->mark_declaration(definition_id);
    return result;
}

cpp_entity_kind cpp_variable::do_get_entity_kind() const noexcept
{
    return kind();
}
