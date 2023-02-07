// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_function_template.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_function_template::kind() noexcept
{
    return cpp_entity_kind::function_template_t;
}

cpp_entity_kind cpp_function_template::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_function_template_specialization::kind() noexcept
{
    return cpp_entity_kind::function_template_specialization_t;
}

cpp_entity_kind cpp_function_template_specialization::do_get_entity_kind() const noexcept
{
    return kind();
}
