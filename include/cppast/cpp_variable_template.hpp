// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_VARIABLE_TEMPLATE_HPP_INCLUDED
#define CPPAST_CPP_VARIABLE_TEMPLATE_HPP_INCLUDED

#include <cppast/cpp_template.hpp>
#include <cppast/cpp_variable.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a C++ alias template.
class cpp_variable_template final : public cpp_template
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_variable_template]().
    class builder : public basic_builder<cpp_variable_template, cpp_variable>
    {
    public:
        using basic_builder::basic_builder;
    };

    /// \returns A reference to the type variable that is being templated.
    const cpp_variable& variable() const noexcept
    {
        return static_cast<const cpp_variable&>(*begin());
    }

private:
    cpp_variable_template(std::unique_ptr<cpp_variable> variable)
    : cpp_template(std::unique_ptr<cpp_entity>(variable.release()))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    friend basic_builder<cpp_variable_template, cpp_variable>;
};
} // namespace cppast

#endif // CPPAST_CPP_VARIABLE_TEMPLATE_HPP_INCLUDED
