// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

    case cpp_entity_kind::macro_parameter_t:
        return "macro parameter";
    case cpp_entity_kind::macro_definition_t:
        return "macro definition";
    case cpp_entity_kind::include_directive_t:
        return "include directive";

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
    case cpp_entity_kind::constructor_t:
        return "constructor";
    case cpp_entity_kind::destructor_t:
        return "destructor";

    case cpp_entity_kind::friend_t:
        return "friend";

    case cpp_entity_kind::template_type_parameter_t:
        return "template type parameter";
    case cpp_entity_kind::non_type_template_parameter_t:
        return "non type template parameter";
    case cpp_entity_kind::template_template_parameter_t:
        return "template template parameter";

    case cpp_entity_kind::alias_template_t:
        return "alias template";
    case cpp_entity_kind::variable_template_t:
        return "variable template";
    case cpp_entity_kind::function_template_t:
        return "function template";
    case cpp_entity_kind::function_template_specialization_t:
        return "function template specialization";
    case cpp_entity_kind::class_template_t:
        return "class template";
    case cpp_entity_kind::class_template_specialization_t:
        return "class template specialization";

    case cpp_entity_kind::static_assert_t:
        return "static_assert";

    case cpp_entity_kind::unexposed_t:
        return "unexposed entity";

    case cpp_entity_kind::count:
        break;
    }

    return "invalid";
}

bool cppast::is_function(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::class_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::class_template_specialization_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
    case cpp_entity_kind::count:
        break;
    }

    return false;
}

bool cppast::is_parameter(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::class_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::class_template_specialization_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
    case cpp_entity_kind::count:
        break;
    }
    return false;
}

bool cppast::is_template(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::class_template_specialization_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::class_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
    case cpp_entity_kind::count:
        break;
    }

    return false;
}

bool cppast::is_template_specialization(cpp_entity_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_specialization_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::class_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
    case cpp_entity_kind::count:
        break;
    }
    return false;
}
