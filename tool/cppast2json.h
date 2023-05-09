#pragma once

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_concept.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_friend.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_language_linkage.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_preprocessor.hpp>
#include <cppast/cpp_static_assert.hpp>
#include <cppast/cpp_template_parameter.hpp>
#include <cppast/cpp_token.hpp>
#include <cppast/cpp_type_alias.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_variable_template.hpp>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace cppast
{
void to_json(json& j, const cpp_class_kind& p);

void to_json(json& j, const cpp_entity_kind& p);

void to_json(json& j, const cpp_include_kind& p);

void to_json(json& j, const cpp_token_kind& p);

void to_json(json& j, const cpp_type_kind& p);

void to_json(json& j, const cpp_scope_name& p);

void to_json(json& j, const cpp_attribute& p);

template <typename T>
void to_json(json& j, const std::vector<T>& p)
{
    j = json::array();
    for (auto& it : p)
    {
        j.push_back(it);
    }
}

template <typename T, typename Predicate>
void to_json(json& j, const basic_cpp_entity_ref<T, Predicate>& p)
{
    j = json{
        {"name", p.name()},
        {"is_overloaded", p.is_overloaded()},
        {"no_overloaded", p.no_overloaded().get()},
        {"id", p.id()},
    };
}

template <typename T>
void to_json(json& j, const type_safe::optional_ref<const T>& p)
{
    if (p.has_value())
    {
        j = p.value();
    }
    else
    {
        j = nullptr;
    }
}

template <typename T>
void to_json(json& j, const type_safe::optional_ref<T>& p)
{
    if (p.has_value())
    {
        j = p.value();
    }
    else
    {
        j = nullptr;
    }
}

template <typename T>
void to_json(json& j, const type_safe::optional<T>& p)
{
    if (p.has_value())
    {
        j = p.value();
    }
    else
    {
        j = nullptr;
    }
}

void to_json(json& j, const cpp_forward_declarable& p);

template <typename T>
void to_json(json& j, const detail::iteratable_intrusive_list<T>& p)
{
    j = json::array();
    for (auto& it : p)
    {
        j.push_back(it);
    }
}

template <typename T>
void to_json(json& j, const type_safe::array_ref<const T>& p)
{
    j = json::array();
    for (auto& it : p)
    {
        j.push_back(it);
    }
}

void to_json(json& j, const cpp_entity_id& p);

void to_json(json& j, const cpp_type& p);

void to_json(json& j, const cpp_expression& p);

void to_json(json& j, const cpp_doc_comment& p);

void to_json(json& j, const cpp_variable_base& p);

void to_json(json& j, const cpp_member_variable_base& p);

void to_json(json& j, const cpp_function_base& p);

void to_json(json& j, const cpp_member_function_base& p);

template <typename T>
void to_json(json& j, const type_safe::flag_set<T>& p)
{
    j = p.template to_int<unsigned>();
}

void to_json(json& j, const cpp_token_string& p);

void to_json(json& j, const cpp_entity& p);

void to_json_x(json& j, const cpp_entity& p);

void to_json(json& j, const cpp_file& p);

void to_json(json& j, const cpp_macro_parameter& p);

void to_json(json& j, const cpp_macro_definition& p);

void to_json(json& j, const cpp_include_directive& p);

void to_json(json& j, const cpp_language_linkage& p);

void to_json(json& j, const cpp_namespace& p);

void to_json(json& j, const cpp_namespace_alias& p);

void to_json(json& j, const cpp_using_directive& p);

void to_json(json& j, const cpp_using_declaration& p);

void to_json(json& j, const cpp_type_alias& p);

void to_json(json& j, const cpp_enum& p);

void to_json(json& j, const cpp_enum_value& p);

void to_json(json& j, const cpp_class& p);

void to_json(json& j, const cpp_access_specifier& p);

void to_json(json& j, const cpp_base_class& p);

void to_json(json& j, const cpp_variable& p);

void to_json(json& j, const cpp_member_variable& p);

void to_json(json& j, const cpp_bitfield& p);

void to_json(json& j, const cpp_function_parameter& p);

void to_json(json& j, const cpp_function& p);

void to_json(json& j, const cpp_member_function& p);

void to_json(json& j, const cpp_conversion_op& p);

void to_json(json& j, const cpp_constructor& p);

void to_json(json& j, const cpp_destructor& p);

void to_json(json& j, const cpp_friend& p);

void to_json(json& j, const cpp_template_type_parameter& p);

void to_json(json& j, const cpp_non_type_template_parameter& p);

void to_json(json& j, const cpp_template_template_parameter& p);

void to_json(json& j, const cpp_alias_template& p);

void to_json(json& j, const cpp_variable_template& p);

void to_json(json& j, const cpp_function_template& p);

void to_json(json& j, const cpp_function_template_specialization& p);

void to_json(json& j, const cpp_class_template& p);

void to_json(json& j, const cpp_class_template_specialization& p);

void to_json(json& j, const cpp_concept& p);

void to_json(json& j, const cpp_static_assert& p);
} // namespace cppast
