// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
