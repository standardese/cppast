// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_template_parameter.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

const char* cppast::to_string(cpp_template_keyword kw) noexcept
{
    switch (kw)
    {
    case cpp_template_keyword::keyword_class:
        return "class";
    case cpp_template_keyword::keyword_typename:
        return "typename";
    }

    return "should not get here";
}

std::unique_ptr<cpp_template_type_parameter> cpp_template_type_parameter::build(
    const cpp_entity_index& idx, cpp_entity_id id, std::string name, cpp_template_keyword kw,
    bool variadic, std::unique_ptr<cpp_type> default_type)
{
    std::unique_ptr<cpp_template_type_parameter> result(
        new cpp_template_type_parameter(std::move(name), kw, variadic, std::move(default_type)));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

cpp_entity_kind cpp_template_type_parameter::kind() noexcept
{
    return cpp_entity_kind::template_type_parameter_t;
}

cpp_entity_kind cpp_template_type_parameter::do_get_entity_kind() const noexcept
{
    return kind();
}

bool detail::cpp_template_parameter_ref_predicate::operator()(const cpp_entity& e)
{
    return e.kind() == cpp_entity_kind::template_type_parameter_t;
}

std::unique_ptr<cpp_non_type_template_parameter> cpp_non_type_template_parameter::build(
    const cpp_entity_index& idx, cpp_entity_id id, std::string name, std::unique_ptr<cpp_type> type,
    bool is_variadic, std::unique_ptr<cpp_expression> default_value)
{
    std::unique_ptr<cpp_non_type_template_parameter> result(
        new cpp_non_type_template_parameter(std::move(name), std::move(type), is_variadic,
                                            std::move(default_value)));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

cpp_entity_kind cpp_non_type_template_parameter::kind() noexcept
{
    return cpp_entity_kind::non_type_template_parameter_t;
}

cpp_entity_kind cpp_non_type_template_parameter::do_get_entity_kind() const noexcept
{
    return kind();
}

bool detail::cpp_template_ref_predicate::operator()(const cpp_entity& e)
{
    return is_template(e.kind()) || e.kind() == cpp_entity_kind::template_template_parameter_t;
}

cpp_entity_kind cpp_template_template_parameter::kind() noexcept
{
    return cpp_entity_kind::template_template_parameter_t;
}

cpp_entity_kind cpp_template_template_parameter::do_get_entity_kind() const noexcept
{
    return kind();
}
