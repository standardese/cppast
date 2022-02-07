// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_CLASS_TEMPLATE_HPP_INCLUDED
#define CPPAST_CPP_CLASS_TEMPLATE_HPP_INCLUDED

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_template.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a class template.
class cpp_class_template final : public cpp_template
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_class_template]().
    class builder : public basic_builder<cpp_class_template, cpp_class>
    {
    public:
        using basic_builder::basic_builder;
    };

    /// A reference to the class that is being templated.
    const cpp_class& class_() const noexcept
    {
        return static_cast<const cpp_class&>(*begin());
    }

private:
    cpp_class_template(std::unique_ptr<cpp_class> func)
    : cpp_template(std::unique_ptr<cpp_entity>(func.release()))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    friend basic_builder<cpp_class_template, cpp_class>;
};

/// A [cppast::cpp_entity]() modelling a class template specialization.
class cpp_class_template_specialization final : public cpp_template_specialization
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_class_template_specialization]().
    class builder : public specialization_builder<cpp_class_template_specialization, cpp_class>
    {
    public:
        using specialization_builder::specialization_builder;
    };

    /// A reference to the class that is being specialized.
    const cpp_class& class_() const noexcept
    {
        return static_cast<const cpp_class&>(*begin());
    }

private:
    cpp_class_template_specialization(std::unique_ptr<cpp_class> func, cpp_template_ref primary)
    : cpp_template_specialization(std::unique_ptr<cpp_entity>(func.release()), primary)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    friend specialization_builder<cpp_class_template_specialization, cpp_class>;
};
} // namespace cppast

#endif // CPPAST_CPP_CLASS_TEMPLATE_HPP_INCLUDED
