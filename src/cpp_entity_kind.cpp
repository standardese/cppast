// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

bool cppast::is_type(cpp_entity_kind type) noexcept
{
    switch (type)
    {
    case cpp_entity_kind::enum_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::count:
        break;
    }

    return false;
}
