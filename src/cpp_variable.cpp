// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_variable.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_variable::kind() noexcept
{
    return cpp_entity_kind::variable_t;
}

std::unique_ptr<cpp_variable> cpp_variable::build(const cpp_entity_index& idx, cpp_entity_id id,
                                                  std::string name, std::unique_ptr<cpp_type> type,
                                                  std::unique_ptr<cpp_expression> def,
                                                  cpp_storage_class_specifiers    spec,
                                                  bool                            is_constexpr)
{
    auto result = std::unique_ptr<cpp_variable>(
        new cpp_variable(std::move(name), std::move(type), std::move(def), spec, is_constexpr));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

std::unique_ptr<cpp_variable> cpp_variable::build_declaration(cpp_entity_id definition_id,
                                                              std::string   name,
                                                              std::unique_ptr<cpp_type>    type,
                                                              cpp_storage_class_specifiers spec,
                                                              bool is_constexpr)
{
    auto result = std::unique_ptr<cpp_variable>(
        new cpp_variable(std::move(name), std::move(type), nullptr, spec, is_constexpr));
    result->mark_declaration(definition_id);
    return result;
}

cpp_entity_kind cpp_variable::do_get_entity_kind() const noexcept
{
    return kind();
}
