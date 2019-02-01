// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
