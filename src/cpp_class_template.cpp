// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_class_template.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_class_template::kind() noexcept
{
    return cpp_entity_kind::class_template_t;
}

cpp_entity_kind cpp_class_template::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_class_template_specialization::kind() noexcept
{
    return cpp_entity_kind::class_template_specialization_t;
}

cpp_entity_kind cpp_class_template_specialization::do_get_entity_kind() const noexcept
{
    return kind();
}
