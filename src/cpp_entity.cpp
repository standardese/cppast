// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity.hpp>

#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_unexposed_entity::kind() noexcept
{
    return cpp_entity_kind::unexposed_t;
}

std::unique_ptr<cpp_entity> cpp_unexposed_entity::build(const cpp_entity_index& index,
                                                        cpp_entity_id id, std::string name,
                                                        std::string spelling)
{
    std::unique_ptr<cpp_entity> result(
        new cpp_unexposed_entity(std::move(name), std::move(spelling)));
    index.register_forward_declaration(id, type_safe::ref(*result));
    return result;
}

std::unique_ptr<cpp_entity> cpp_unexposed_entity::build(std::string spelling)
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
