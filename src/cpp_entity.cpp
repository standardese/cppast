// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity.hpp>

#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_template.hpp>

using namespace cppast;

cpp_scope_name::cpp_scope_name(type_safe::object_ref<const cpp_entity> entity) : entity_(entity)
{
    if (cppast::is_templated(*entity))
    {
        auto& templ = static_cast<const cpp_template&>(entity->parent().value());
        if (!templ.parameters().empty())
            templ_ = type_safe::ref(templ);
    }
    else if (is_template(entity->kind()))
    {
        auto& templ = static_cast<const cpp_template&>(*entity);
        if (!templ.parameters().empty())
            templ_ = type_safe::ref(templ);
    }
}

const std::string& cpp_scope_name::name() const noexcept
{
    return entity_->name();
}

detail::iteratable_intrusive_list<cpp_template_parameter> cpp_scope_name::template_parameters()
    const noexcept
{
    DEBUG_ASSERT(is_templated(), detail::precondition_error_handler{});
    return templ_.value().parameters();
}

cpp_entity_kind cpp_unexposed_entity::kind() noexcept
{
    return cpp_entity_kind::unexposed_t;
}

std::unique_ptr<cpp_entity> cpp_unexposed_entity::build(const cpp_entity_index& index,
                                                        cpp_entity_id id, std::string name,
                                                        cpp_token_string spelling)
{
    std::unique_ptr<cpp_entity> result(
        new cpp_unexposed_entity(std::move(name), std::move(spelling)));
    index.register_forward_declaration(id, type_safe::ref(*result));
    return result;
}

std::unique_ptr<cpp_entity> cpp_unexposed_entity::build(cpp_token_string spelling)
{
    return std::unique_ptr<cpp_entity>(new cpp_unexposed_entity("", std::move(spelling)));
}

cpp_entity_kind cpp_unexposed_entity::do_get_entity_kind() const noexcept
{
    return kind();
}

bool cppast::is_templated(const cpp_entity& e) noexcept
{
    if (!e.parent())
        return false;
    else if (!is_template(e.parent().value().kind()))
        return false;
    return e.parent().value().name() == e.name();
}
