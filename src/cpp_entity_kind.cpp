// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

const char* cppast::to_string(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::file_t:
        return "file";

    case cpp_entity_kind::language_linkage_t:
        return "language linkage";

    case cpp_entity_kind::namespace_t:
        return "namespace";
    case cpp_entity_kind::namespace_alias_t:
        return "namespace alias";
    case cpp_entity_kind::using_directive_t:
        return "using directive";
    case cpp_entity_kind::using_declaration_t:
        return "using declaration";

    case cpp_entity_kind::type_alias_t:
        return "type alias";

    case cpp_entity_kind::enum_t:
        return "enum";
    case cpp_entity_kind::enum_value_t:
        return "enum value";

    case cpp_entity_kind::class_t:
        return "class";
    case cpp_entity_kind::access_specifier_t:
        return "access specifier";
    case cpp_entity_kind::base_class_t:
        return "base class specifier";

    case cpp_entity_kind::variable_t:
        return "variable";
    case cpp_entity_kind::member_variable_t:
        return "member variable";
    case cpp_entity_kind::bitfield_t:
        return "bit field";

    case cpp_entity_kind::function_parameter_t:
        return "function parameter";
    case cpp_entity_kind::function_t:
        return "function";
    case cpp_entity_kind::member_function_t:
        return "member function";
    case cpp_entity_kind::conversion_op_t:
        return "conversion operator";

    case cpp_entity_kind::count:
        break;
    }

    return "invalid";
}

bool cppast::is_type(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::class_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::count:
        break;
    }

    return false;
}
