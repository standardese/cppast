// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_STATIC_ASSERT_HPP_INCLUDED
#define CPPAST_CPP_STATIC_ASSERT_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_expression.hpp>

namespace cppast
{
class cpp_static_assert : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created `static_assert()` entity.
    /// \notes It will not be registered as nothing can refer to it.
    static std::unique_ptr<cpp_static_assert> build(std::unique_ptr<cpp_expression> expr,
                                                    std::string                     msg)
    {
        return std::unique_ptr<cpp_static_assert>(
            new cpp_static_assert(std::move(expr), std::move(msg)));
    }

    /// \returns A reference to the [cppast::cpp_expression]() that is being asserted.
    const cpp_expression& expression() const noexcept
    {
        return *expr_;
    }

    /// \returns A reference to the message of the assertion.
    const std::string& message() const noexcept
    {
        return msg_;
    }

private:
    cpp_static_assert(std::unique_ptr<cpp_expression> expr, std::string msg)
    : cpp_entity(""), expr_(std::move(expr)), msg_(std::move(msg))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    std::unique_ptr<cpp_expression> expr_;
    std::string                     msg_;
};
} // namespace cppast

#endif // CPPAST_CPP_STATIC_ASSERT_HPP_INCLUDED
