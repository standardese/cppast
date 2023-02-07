// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_static_assert.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cppast::cpp_entity_kind cpp_static_assert::kind() noexcept
{
    return cpp_entity_kind::static_assert_t;
}

cppast::cpp_entity_kind cpp_static_assert::do_get_entity_kind() const noexcept
{
    return kind();
}
