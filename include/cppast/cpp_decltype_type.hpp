// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_DECLTYPE_TYPE_HPP_INCLUDED
#define CPPAST_CPP_DECLTYPE_TYPE_HPP_INCLUDED

#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
/// A [cppast::cpp_type]() that isn't given but taken from an expression.
class cpp_decltype_type final : public cpp_type
{
public:
    /// \returns A newly created `decltype` type.
    static std::unique_ptr<cpp_decltype_type> build(std::unique_ptr<cpp_expression> expr)
    {
        return std::unique_ptr<cpp_decltype_type>(new cpp_decltype_type(std::move(expr)));
    }

    /// \returns A reference to the expression given.
    const cpp_expression& expression() const noexcept
    {
        return *expr_;
    }

private:
    cpp_decltype_type(std::unique_ptr<cpp_expression> expr) : expr_(std::move(expr)) {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::decltype_t;
    }

    std::unique_ptr<cpp_expression> expr_;
};

/// A [cppast::cpp_type]() that isn't given but deduced using the `decltype` rules.
class cpp_decltype_auto_type final : public cpp_type
{
public:
    /// \returns A newly created `auto` type.
    static std::unique_ptr<cpp_decltype_auto_type> build()
    {
        return std::unique_ptr<cpp_decltype_auto_type>(new cpp_decltype_auto_type);
    }

private:
    cpp_decltype_auto_type() = default;

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::decltype_auto_t;
    }
};
} // namespace cppast

#endif // CPPAST_CPP_DECLTYPE_TYPE_HPP_INCLUDED
