// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_VARIABLE_BASE_HPP_INCLUDED
#define CPPAST_CPP_VARIABLE_BASE_HPP_INCLUDED

#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
/// Additional base class for all [cppast::cpp_entity]() modelling some kind of variable.
///
/// Examples are [cppast::cpp_variable]() or [cppast::cpp_function_parameter](),
/// or anything that is name/type/default-value triple.
class cpp_variable_base
{
public:
    /// \returns A reference to the [cppast::cpp_type]() of the variable.
    const cpp_type& type() const noexcept
    {
        return *type_;
    }

    /// \returns A [ts::optional_ref]() to the [cppast::cpp_expression]() that is the default value.
    type_safe::optional_ref<const cpp_expression> default_value() const noexcept
    {
        return type_safe::opt_ref(default_.get());
    }

protected:
    cpp_variable_base(std::unique_ptr<cpp_type> type, std::unique_ptr<cpp_expression> def)
    : type_(std::move(type)), default_(std::move(def))
    {}

    ~cpp_variable_base() noexcept = default;

private:
    std::unique_ptr<cpp_type>       type_;
    std::unique_ptr<cpp_expression> default_;
};
} // namespace cppast

#endif // CPPAST_CPP_VARIABLE_BASE_HPP_INCLUDED
