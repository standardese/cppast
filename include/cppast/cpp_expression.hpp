// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_EXPRESSION_HPP_INCLUDED
#define CPPAST_CPP_EXPRESSION_HPP_INCLUDED

#include <atomic>
#include <memory>

#include <cppast/cpp_token.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
/// The kind of a [cppast::cpp_expression]().
enum class cpp_expression_kind
{
    literal_t,

    unexposed_t,
};

/// Base class for all C++ expressions.
class cpp_expression
{
public:
    cpp_expression(const cpp_expression&) = delete;
    cpp_expression& operator=(const cpp_expression&) = delete;

    virtual ~cpp_expression() noexcept = default;

    /// \returns The [cppast::cpp_expression_kind]().
    cpp_expression_kind kind() const noexcept
    {
        return do_get_kind();
    }

    /// \returns The type of the expression.
    const cpp_type& type() const noexcept
    {
        return *type_;
    }

    /// \returns The specified user data.
    void* user_data() const noexcept
    {
        return user_data_.load();
    }

    /// \effects Sets some kind of user data.
    ///
    /// User data is just some kind of pointer, there are no requirements.
    /// The class will do no lifetime management.
    ///
    /// User data is useful if you need to store additional data for an entity without the need to
    /// maintain a registry.
    void set_user_data(void* data) const noexcept
    {
        user_data_ = data;
    }

protected:
    /// \effects Creates it given the type.
    /// \requires The type must not be `nullptr`.
    cpp_expression(std::unique_ptr<cpp_type> type) : type_(std::move(type)), user_data_(nullptr)
    {
        DEBUG_ASSERT(type_ != nullptr, detail::precondition_error_handler{});
    }

private:
    /// \returns The [cppast::cpp_expression_kind]().
    virtual cpp_expression_kind do_get_kind() const noexcept = 0;

    std::unique_ptr<cpp_type>  type_;
    mutable std::atomic<void*> user_data_;
};

/// An unexposed [cppast::cpp_expression]().
///
/// There is no further information than a string available.
class cpp_unexposed_expression final : public cpp_expression
{
public:
    /// \returns A newly created unexposed expression.
    static std::unique_ptr<cpp_unexposed_expression> build(std::unique_ptr<cpp_type> type,
                                                           cpp_token_string          str)
    {
        return std::unique_ptr<cpp_unexposed_expression>(
            new cpp_unexposed_expression(std::move(type), std::move(str)));
    }

    /// \returns The expression as a string.
    const cpp_token_string& expression() const noexcept
    {
        return str_;
    }

private:
    cpp_unexposed_expression(std::unique_ptr<cpp_type> type, cpp_token_string str)
    : cpp_expression(std::move(type)), str_(std::move(str))
    {}

    cpp_expression_kind do_get_kind() const noexcept override
    {
        return cpp_expression_kind::unexposed_t;
    }

    cpp_token_string str_;
};

/// A [cppast::cpp_expression]() that is a literal.
class cpp_literal_expression final : public cpp_expression
{
public:
    /// \returns A newly created literal expression.
    static std::unique_ptr<cpp_literal_expression> build(std::unique_ptr<cpp_type> type,
                                                         std::string               value)
    {
        return std::unique_ptr<cpp_literal_expression>(
            new cpp_literal_expression(std::move(type), std::move(value)));
    }

    /// \returns The value of the literal, as string.
    const std::string& value() const noexcept
    {
        return value_;
    }

private:
    cpp_literal_expression(std::unique_ptr<cpp_type> type, std::string value)
    : cpp_expression(std::move(type)), value_(std::move(value))
    {}

    cpp_expression_kind do_get_kind() const noexcept override
    {
        return cpp_expression_kind::literal_t;
    }

    std::string value_;
};

/// \exclude
namespace detail
{
    void write_expression(code_generator::output& output, const cpp_expression& expr);
} // namespace detail
} // namespace cppast

#endif // CPPAST_CPP_EXPRESSION_HPP_INCLUDED
