// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_FUNCTION_TYPE_HPP_INCLUDED
#define CPPAST_CPP_FUNCTION_TYPE_HPP_INCLUDED

#include <cppast/cpp_type.hpp>

namespace cppast
{
/// A [cppast::cpp_type]() that is a function.
///
/// A function pointer is created by wrapping it in [cppast::cpp_pointer_type]().
class cpp_function_type final : public cpp_type
{
public:
    /// Builds a [cppast::cpp_function_type]().
    class builder
    {
    public:
        /// \effects Sets the return type.
        explicit builder(std::unique_ptr<cpp_type> return_type)
        : func_(new cpp_function_type(std::move(return_type)))
        {}

        /// \effects Adds an parameter type.
        void add_parameter(std::unique_ptr<cpp_type> arg)
        {
            func_->parameters_.push_back(*func_, std::move(arg));
        }

        /// \effects Adds an ellipsis, marking it as variadic.
        void is_variadic()
        {
            func_->variadic_ = true;
        }

        /// \returns The finished [cppast::cpp_function_type]().
        std::unique_ptr<cpp_function_type> finish()
        {
            return std::move(func_);
        }

    private:
        std::unique_ptr<cpp_function_type> func_;
    };

    /// \returns A reference to the return [cppast::cpp_type]().
    const cpp_type& return_type() const noexcept
    {
        return *return_type_;
    }

    /// \returns An iteratable object iterating over the parameter types.
    detail::iteratable_intrusive_list<cpp_type> parameter_types() const noexcept
    {
        return type_safe::ref(parameters_);
    }

    /// \returns Whether or not the function is variadic (C-style ellipsis).
    bool is_variadic() const noexcept
    {
        return variadic_;
    }

private:
    cpp_function_type(std::unique_ptr<cpp_type> return_type)
    : return_type_(std::move(return_type)), variadic_(false)
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::function_t;
    }

    std::unique_ptr<cpp_type>        return_type_;
    detail::intrusive_list<cpp_type> parameters_;
    bool                             variadic_;
};

/// A [cppast::cpp_type]() that is a member function.
///
/// A member function with cv qualifier is created by wrapping it in
/// [cppast::cpp_cv_qualified_type](). A member function with reference qualifier is created by
/// wrapping it in [cppast::cpp_reference_type](). A member function pointer is created by wrapping
/// it in [cppast::cpp_pointer_type]().
class cpp_member_function_type final : public cpp_type
{
public:
    /// Builds a [cppast::cpp_member_function_type]().
    class builder
    {
    public:
        /// \effects Sets the class and return type.
        builder(std::unique_ptr<cpp_type> class_type, std::unique_ptr<cpp_type> return_type)
        : func_(new cpp_member_function_type(std::move(class_type), std::move(return_type)))
        {}

        /// \effects Adds a parameter type.
        void add_parameter(std::unique_ptr<cpp_type> arg)
        {
            func_->parameters_.push_back(*func_, std::move(arg));
        }

        /// \effects Adds an ellipsis, marking it as variadic.
        void is_variadic()
        {
            func_->variadic_ = true;
        }

        /// \returns The finished [cppast::cpp_member_function_type]().
        std::unique_ptr<cpp_member_function_type> finish()
        {
            return std::move(func_);
        }

    private:
        std::unique_ptr<cpp_member_function_type> func_;
    };

    /// \returns A reference to the class [cppast::cpp_type]().
    const cpp_type& class_type() const noexcept
    {
        return *class_type_;
    }

    /// \returns A reference to the return [cppast::cpp_type]().
    const cpp_type& return_type() const noexcept
    {
        return *return_type_;
    }

    /// \returns An iteratable object iterating over the parameter types.
    detail::iteratable_intrusive_list<cpp_type> parameter_types() const noexcept
    {
        return type_safe::ref(parameters_);
    }

    /// \returns Whether or not the function is variadic (C-style ellipsis).
    bool is_variadic() const noexcept
    {
        return variadic_;
    }

private:
    cpp_member_function_type(std::unique_ptr<cpp_type> class_type,
                             std::unique_ptr<cpp_type> return_type)
    : class_type_(std::move(class_type)), return_type_(std::move(return_type)), variadic_(false)
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::member_function_t;
    }

    std::unique_ptr<cpp_type>        class_type_, return_type_;
    detail::intrusive_list<cpp_type> parameters_;
    bool                             variadic_;
};

/// A [cppast::cpp_type]() that is a member object.
///
/// A member object pointer is created by wrapping it in [cppast::cpp_pointer_type]().
class cpp_member_object_type final : public cpp_type
{
public:
    /// \returns A newly created member object type.
    static std::unique_ptr<cpp_member_object_type> build(std::unique_ptr<cpp_type> class_type,
                                                         std::unique_ptr<cpp_type> object_type)
    {
        return std::unique_ptr<cpp_member_object_type>(
            new cpp_member_object_type(std::move(class_type), std::move(object_type)));
    }

    /// \returns A reference to the class [cppast::cpp_type]().
    const cpp_type& class_type() const noexcept
    {
        return *class_type_;
    }

    /// \returns A reference to the object [cppast::cpp_type]().
    const cpp_type& object_type() const noexcept
    {
        return *object_type_;
    }

private:
    cpp_member_object_type(std::unique_ptr<cpp_type> class_type,
                           std::unique_ptr<cpp_type> object_type)
    : class_type_(std::move(class_type)), object_type_(std::move(object_type))
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::member_object_t;
    }

    std::unique_ptr<cpp_type> class_type_, object_type_;
};
} // namespace cppast

#endif // CPPAST_CPP_FUNCTION_TYPE_HPP_INCLUDED
