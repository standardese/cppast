// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

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
