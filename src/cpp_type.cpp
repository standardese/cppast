// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_template.hpp>

using namespace cppast;

bool detail::cpp_type_ref_predicate::operator()(const cpp_entity& e)
{
    return is_type(e.kind());
}

std::unique_ptr<cpp_dependent_type> cpp_dependent_type::build(
    std::string name, std::unique_ptr<cpp_template_parameter_type> dependee)
{
    return std::unique_ptr<cpp_dependent_type>(
        new cpp_dependent_type(std::move(name), std::move(dependee)));
}

std::unique_ptr<cpp_dependent_type> cpp_dependent_type::build(
    std::string name, std::unique_ptr<cpp_template_instantiation_type> dependee)
{
    return std::unique_ptr<cpp_dependent_type>(
        new cpp_dependent_type(std::move(name), std::move(dependee)));
}
