// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_preprocessor.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_macro_parameter::kind() noexcept
{
    return cpp_entity_kind::macro_parameter_t;
}

cpp_entity_kind cpp_macro_parameter::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_macro_definition::kind() noexcept
{
    return cpp_entity_kind::macro_definition_t;
}

cpp_entity_kind cpp_macro_definition::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_include_directive::kind() noexcept
{
    return cpp_entity_kind::include_directive_t;
}

cpp_entity_kind cpp_include_directive::do_get_entity_kind() const noexcept
{
    return kind();
}
