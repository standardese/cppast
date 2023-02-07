// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_friend.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_friend::kind() noexcept
{
    return cpp_entity_kind::friend_t;
}

cpp_entity_kind cpp_friend::do_get_entity_kind() const noexcept
{
    return kind();
}

bool cppast::is_friended(const cpp_entity& e) noexcept
{
    if (is_templated(e))
        return is_friended(e.parent().value());
    return e.parent() && e.parent().value().kind() == cpp_entity_kind::friend_t;
}
