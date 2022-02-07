// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_ALIAS_TEMPLATE_HPP_INCLUDED
#define CPPAST_CPP_ALIAS_TEMPLATE_HPP_INCLUDED

#include <cppast/cpp_template.hpp>
#include <cppast/cpp_type_alias.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a C++ alias template.
class cpp_alias_template final : public cpp_template
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_alias_template]().
    class builder : public basic_builder<cpp_alias_template, cpp_type_alias>
    {
    public:
        using basic_builder::basic_builder;
    };

    /// \returns A reference to the type alias that is being templated.
    const cpp_type_alias& type_alias() const noexcept
    {
        return static_cast<const cpp_type_alias&>(*begin());
    }

private:
    cpp_alias_template(std::unique_ptr<cpp_type_alias> alias)
    : cpp_template(std::unique_ptr<cpp_entity>(alias.release()))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    friend basic_builder<cpp_alias_template, cpp_type_alias>;
};
} // namespace cppast

#endif // CPPAST_CPP_ALIAS_TEMPLATE_HPP_INCLUDED
