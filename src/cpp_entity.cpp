// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

bool cppast::is_templated(const cpp_entity& e) noexcept
{
    if (!e.parent())
        return false;
    else if (!is_template(e.parent().value().kind()))
        return false;
    return e.parent().value().name() == e.name();
}

std::string cppast::full_name(const cpp_entity& e)
{
    if (e.name().empty())
        return "";
    else if (is_parameter(e.kind()))
        // parameters don't have a full name
        return e.name();

    std::string scopes;

    for (auto cur = e.parent(); cur; cur = cur.value().parent())
        // prepend each scope, if there is any
        type_safe::with(cur.value().scope_name(),
                        [&](const std::string& cur_scope) { scopes = cur_scope + "::" + scopes; });

    return scopes + e.name();
}
