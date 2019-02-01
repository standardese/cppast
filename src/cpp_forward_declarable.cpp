// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_forward_declarable.hpp>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_variable.hpp>

using namespace cppast;

namespace
{
type_safe::optional_ref<const cpp_forward_declarable> get_declarable(const cpp_entity& e)
{
    switch (e.kind())
    {
    case cpp_entity_kind::enum_t:
        return type_safe::ref(static_cast<const cpp_enum&>(e));
    case cpp_entity_kind::class_t:
        return type_safe::ref(static_cast<const cpp_class&>(e));
    case cpp_entity_kind::variable_t:
        return type_safe::ref(static_cast<const cpp_variable&>(e));
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
        return type_safe::ref(static_cast<const cpp_function_base&>(e));
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::class_template_specialization_t:
        return get_declarable(*static_cast<const cpp_template&>(e).begin());

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
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
        return nullptr;

    case cpp_entity_kind::count:
        break;
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return nullptr;
}

type_safe::optional_ref<const cpp_entity> get_definition_impl(const cpp_entity_index& idx,
                                                              const cpp_entity&       e)
{
    auto declarable = get_declarable(e);
    if (!declarable || declarable.value().is_definition())
        // not declarable or is a definition
        // return reference to entity itself
        return type_safe::ref(e);
    // else lookup definition
    return idx.lookup_definition(declarable.value().definition().value());
}
} // namespace

bool cppast::is_definition(const cpp_entity& e) noexcept
{
    auto declarable = get_declarable(e);
    return declarable && declarable.value().is_definition();
}

type_safe::optional_ref<const cpp_entity> cppast::get_definition(const cpp_entity_index& idx,
                                                                 const cpp_entity&       e)
{
    return get_definition_impl(idx, e);
}

type_safe::optional_ref<const cpp_enum> cppast::get_definition(const cpp_entity_index& idx,
                                                               const cpp_enum&         e)
{
    return get_definition_impl(idx, e).map([](const cpp_entity& e) {
        DEBUG_ASSERT(e.kind() == cpp_entity_kind::enum_t, detail::assert_handler{});
        return type_safe::ref(static_cast<const cpp_enum&>(e));
    });
}

type_safe::optional_ref<const cpp_class> cppast::get_definition(const cpp_entity_index& idx,
                                                                const cpp_class&        e)
{
    return get_definition_impl(idx, e).map([](const cpp_entity& e) {
        DEBUG_ASSERT(e.kind() == cpp_entity_kind::class_t, detail::assert_handler{});
        return type_safe::ref(static_cast<const cpp_class&>(e));
    });
}

type_safe::optional_ref<const cpp_variable> cppast::get_definition(const cpp_entity_index& idx,
                                                                   const cpp_variable&     e)
{
    return get_definition_impl(idx, e).map([](const cpp_entity& e) {
        DEBUG_ASSERT(e.kind() == cpp_entity_kind::variable_t, detail::assert_handler{});
        return type_safe::ref(static_cast<const cpp_variable&>(e));
    });
}

type_safe::optional_ref<const cpp_function_base> cppast::get_definition(const cpp_entity_index& idx,
                                                                        const cpp_function_base& e)
{
    return get_definition_impl(idx, e).map([](const cpp_entity& e) {
        DEBUG_ASSERT(is_function(e.kind()), detail::assert_handler{});
        return type_safe::ref(static_cast<const cpp_function_base&>(e));
    });
}
