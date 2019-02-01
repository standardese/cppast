// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/visitor.hpp>

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_language_linkage.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_variable_template.hpp>

using namespace cppast;

namespace
{
cpp_access_specifier_kind get_initial_access(const cpp_entity& e)
{
    if (e.kind() == cpp_class::kind())
        return static_cast<const cpp_class&>(e).class_kind() == cpp_class_kind::class_t
                   ? cpp_private
                   : cpp_public;
    return cpp_public;
}

void update_access(cpp_access_specifier_kind& child_access, const cpp_entity& child)
{
    if (child.kind() == cpp_access_specifier::kind())
        child_access = static_cast<const cpp_access_specifier&>(child).access_specifier();
}

template <typename T>
bool handle_container(const cpp_entity& e, detail::visitor_callback_t cb, void* functor,
                      cpp_access_specifier_kind cur_access, bool last_child)
{
    auto& container = static_cast<const T&>(e);

    auto handle_children
        = cb(functor, container, {visitor_info::container_entity_enter, cur_access, last_child});
    if (handle_children)
    {
        auto child_access = get_initial_access(e);
        for (auto iter = container.begin(); iter != container.end();)
        {
            auto& cur = *iter;
            ++iter;

            update_access(child_access, cur);

            if (!detail::visit(cur, cb, functor, child_access, iter == container.end()))
                return false;
        }
    }

    return cb(functor, container, {visitor_info::container_entity_exit, cur_access, last_child});
}
} // namespace

bool detail::visit(const cpp_entity& e, detail::visitor_callback_t cb, void* functor,
                   cpp_access_specifier_kind cur_access, bool last_child)
{
    switch (e.kind())
    {
    case cpp_entity_kind::file_t:
        return handle_container<cpp_file>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::language_linkage_t:
        return handle_container<cpp_language_linkage>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::namespace_t:
        return handle_container<cpp_namespace>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::enum_t:
        return handle_container<cpp_enum>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::class_t:
        return handle_container<cpp_class>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::alias_template_t:
        return handle_container<cpp_alias_template>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::variable_template_t:
        return handle_container<cpp_variable_template>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::function_template_t:
        return handle_container<cpp_function_template>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::function_template_specialization_t:
        return handle_container<cpp_function_template_specialization>(e, cb, functor, cur_access,
                                                                      last_child);
    case cpp_entity_kind::class_template_t:
        return handle_container<cpp_class_template>(e, cb, functor, cur_access, last_child);
    case cpp_entity_kind::class_template_specialization_t:
        return handle_container<cpp_class_template_specialization>(e, cb, functor, cur_access,
                                                                   last_child);

    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::type_alias_t:
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
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
        return cb(functor, e, {visitor_info::leaf_entity, cur_access, last_child});

    case cpp_entity_kind::count:
        break;
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return true;
}
