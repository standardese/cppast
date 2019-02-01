// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
