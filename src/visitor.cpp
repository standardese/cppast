// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/visitor.hpp>

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_language_linkage.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_template_parameter.hpp>
#include <cppast/cpp_variable_template.hpp>

using namespace cppast;

namespace
{
    template <typename T>
    bool handle_container(const cpp_entity& e, detail::visitor_callback_t cb, void* functor)
    {
        auto& container = static_cast<const T&>(e);

        auto handle_children = cb(functor, container, visitor_info::container_entity_enter);
        if (handle_children)
        {
            for (auto& child : container)
                if (!detail::visit(child, cb, functor))
                    return false;
        }

        return cb(functor, container, visitor_info::container_entity_exit);
    }
}

bool detail::visit(const cpp_entity& e, detail::visitor_callback_t cb, void* functor)
{
    switch (e.kind())
    {
    case cpp_entity_kind::file_t:
        return handle_container<cpp_file>(e, cb, functor);
    case cpp_entity_kind::language_linkage_t:
        return handle_container<cpp_language_linkage>(e, cb, functor);
    case cpp_entity_kind::namespace_t:
        return handle_container<cpp_namespace>(e, cb, functor);
    case cpp_entity_kind::enum_t:
        return handle_container<cpp_enum>(e, cb, functor);
    case cpp_entity_kind::class_t:
        return handle_container<cpp_class>(e, cb, functor);
    case cpp_entity_kind::function_t:
        return handle_container<cpp_function>(e, cb, functor);
    case cpp_entity_kind::member_function_t:
        return handle_container<cpp_member_function>(e, cb, functor);
    case cpp_entity_kind::conversion_op_t:
        return handle_container<cpp_conversion_op>(e, cb, functor);
    case cpp_entity_kind::constructor_t:
        return handle_container<cpp_constructor>(e, cb, functor);
    case cpp_entity_kind::template_template_parameter_t:
        return handle_container<cpp_template_template_parameter>(e, cb, functor);
    case cpp_entity_kind::alias_template_t:
        return handle_container<cpp_alias_template>(e, cb, functor);
    case cpp_entity_kind::variable_template_t:
        return handle_container<cpp_variable_template>(e, cb, functor);
    case cpp_entity_kind::function_template_t:
        return handle_container<cpp_function_template>(e, cb, functor);
    case cpp_entity_kind::function_template_specialization_t:
        return handle_container<cpp_function_template_specialization>(e, cb, functor);
    case cpp_entity_kind::class_template_t:
        return handle_container<cpp_class_template>(e, cb, functor);
    case cpp_entity_kind::class_template_specialization_t:
        return handle_container<cpp_class_template_specialization>(e, cb, functor);

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
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
        return cb(functor, e, visitor_info::leaf_entity);

    case cpp_entity_kind::count:
        break;
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return true;
}
