// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
    idx.register_entity(std::move(id), type_safe::cref(*result));
    return result;
}

cpp_entity_kind cpp_variable::do_get_entity_kind() const noexcept
{
    return kind();
}
