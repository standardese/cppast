// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED
#define CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED

#include <type_safe/flag_set.hpp>

#include <cppast/cpp_function.hpp>

namespace cppast
{
/// The `virtual`-ness of a member function.
///
/// This is a [ts::flag_set]() `enum`.
/// \notes It does not specify whether a member function is `virtual` or not,
/// only the kind of `virtual`.
/// \notes As surprising as it may be, any of these can be used in combination,
/// i.e. you can have a `final` non-overriding function or an overriding pure `virtual` function.
enum class cpp_virtual_flags
{
    pure,     //< Set if the function is pure.
    override, //< Set if the function overrides a base class function.
    final,    //< Set if the function is marked `final`.

    _flag_set_size, //< \exclude
};

/// The `virtual` information of a member function.
///
/// This is an optional of the combination of the [cppast::cpp_virtual_flags]().
/// If the optional has a value, the member function is `virtual`,
/// and the [ts::flag_set]() describes additional information.
using cpp_virtual = type_safe::optional<type_safe::flag_set<cpp_virtual_flags>>;

/// \returns Whether or not a member function is `virtual`.
inline bool is_virtual(const cpp_virtual& virt) noexcept
{
    return virt.has_value();
}

/// \returns Whether or not a member function is pure.
inline bool is_pure(const cpp_virtual& virt) noexcept
{
    return static_cast<bool>(virt.value_or(cpp_virtual_flags::final) & cpp_virtual_flags::pure);
}

/// \returns Whether or not a member function overrides another one.
inline bool is_overriding(const cpp_virtual& virt) noexcept
{
    return static_cast<bool>(virt.value_or(cpp_virtual_flags::pure) & cpp_virtual_flags::override);
}

/// \returns Whether or not a member function is `final`.
inline bool is_final(const cpp_virtual& virt) noexcept
{
    return static_cast<bool>(virt.value_or(cpp_virtual_flags::pure) & cpp_virtual_flags::final);
}

/// Base classes for all regular member function.
///
/// The two derived classes are [cppast::cpp_member_function]() and [cppast::cpp_conversion_op]().
class cpp_member_function_base : public cpp_function_base
{
public:
    /// \returns The return type of the member function.
    const cpp_type& return_type() const noexcept
    {
        return *return_type_;
    }

    /// \returns Whether or not it is `virtual`.
    bool is_virtual() const noexcept
    {
        return virtual_info().has_value();
    }

    /// \returns The `virtual`-ness of the member function.
    const cpp_virtual& virtual_info() const noexcept
    {
        return virtual_;
    }

    /// \returns The cv-qualifier on the member function.
    cpp_cv cv_qualifier() const noexcept
    {
        return cv_;
    }

    /// \returns The ref-qualifier on the member function.
    cpp_reference ref_qualifier() const noexcept
    {
        return ref_;
    }

    /// \returns Whether or not the member function is `constexpr`.
    bool is_constexpr() const noexcept
    {
        return constexpr_;
    }

    /// \returns Whether or not the member function is `consteval`.
    bool is_consteval() const noexcept
    {
        return consteval_;
    }

protected:
    /// Builder class for member functions.
    template <typename T>
    class basic_member_builder : public basic_builder<T>
    {
    public:
        /// \effects Sets the name and return type.
        basic_member_builder(std::string name, std::unique_ptr<cpp_type> return_type)
        {
            this->function = std::unique_ptr<T>(new T(std::move(name), std::move(return_type)));
        }

        /// \effects Sets the cv- and ref-qualifier.
        void cv_ref_qualifier(cpp_cv cv, cpp_reference ref) noexcept
        {
            auto& base = static_cast<cpp_member_function_base&>(*this->function);
            base.cv_   = cv;
            base.ref_  = ref;
        }

        /// \effects Sets the `virtual`-ness of the function.
        void virtual_info(type_safe::flag_set<cpp_virtual_flags> virt) noexcept
        {
            static_cast<cpp_member_function_base&>(*this->function).virtual_ = virt;
        }

        /// \effects Marks the function as `constexpr`.
        void is_constexpr() noexcept
        {
            static_cast<cpp_member_function_base&>(*this->function).constexpr_ = true;
        }

        /// \effects Marks the function as `consteval`.
        void is_consteval() noexcept
        {
            static_cast<cpp_member_function_base&>(*this->function).consteval_ = true;
        }

    protected:
        basic_member_builder() noexcept = default;
    };

    /// \effects Sets name and return type, as well as the rest to defaults.
    cpp_member_function_base(std::string name, std::unique_ptr<cpp_type> return_type)
    : cpp_function_base(std::move(name)), return_type_(std::move(return_type)), cv_(cpp_cv_none),
      ref_(cpp_ref_none), constexpr_(false), consteval_(false)
    {}

protected:
    std::string do_get_signature() const override;

private:
    std::unique_ptr<cpp_type> return_type_;
    cpp_virtual               virtual_;
    cpp_cv                    cv_;
    cpp_reference             ref_;
    bool                      constexpr_;
    bool                      consteval_;
};

/// A [cppast::cpp_entity]() modelling a member function.
class cpp_member_function final : public cpp_member_function_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_member_function]().
    class builder : public cpp_member_function_base::basic_member_builder<cpp_member_function>
    {
    public:
        using cpp_member_function_base::basic_member_builder<
            cpp_member_function>::basic_member_builder;
    };

private:
    using cpp_member_function_base::cpp_member_function_base;

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    friend basic_member_builder<cpp_member_function>;
};

/// A [cppast::cpp_entity]() modelling a C++ conversion operator.
class cpp_conversion_op final : public cpp_member_function_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_conversion_op]().
    class builder : public basic_member_builder<cpp_conversion_op>
    {
    public:
        using basic_member_builder::basic_member_builder;

        /// \effects Marks the conversion operator `explicit`.
        void is_explicit() noexcept
        {
            function->explicit_ = true;
        }

    private:
        using basic_member_builder::add_parameter;
        using basic_member_builder::is_variadic;
    };

    /// \returns Whether or not the conversion is `explicit`.
    bool is_explicit() const noexcept
    {
        return explicit_;
    }

private:
    cpp_conversion_op(std::string name, std::unique_ptr<cpp_type> return_t)
    : cpp_member_function_base(std::move(name), std::move(return_t)), explicit_(false)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    bool explicit_;

    friend basic_member_builder<cpp_conversion_op>;
};

/// A [cppast::cpp_entity]() modelling a C++ constructor.
class cpp_constructor final : public cpp_function_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builder for [cppast::cpp_constructor]().
    class builder : public basic_builder<cpp_constructor>
    {
    public:
        using basic_builder::basic_builder;

        /// \effects Marks the constructor `explicit`.
        void is_explicit() noexcept
        {
            function->explicit_ = true;
        }

        /// \effects Marks the constructor `constexpr`.
        void is_constexpr() noexcept
        {
            function->constexpr_ = true;
        }

        /// \effects Marks the constructor `consteval`.
        void is_consteval() noexcept
        {
            function->consteval_ = true;
        }
    };

    /// \returns Whether or not the constructor is `explicit`.
    bool is_explicit() const noexcept
    {
        return explicit_;
    }

    /// \returns Whether or not the constructor is `constexpr`.
    bool is_constexpr() const noexcept
    {
        return constexpr_;
    }

    /// \returns Whether or not the constructor is `consteval`.
    bool is_consteval() const noexcept
    {
        return consteval_;
    }

private:
    cpp_constructor(std::string name)
    : cpp_function_base(std::move(name)), explicit_(false), constexpr_(false), consteval_(false)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    bool explicit_;
    bool constexpr_;
    bool consteval_;

    friend basic_builder<cpp_constructor>;
};

/// A [cppast::cpp_entity]() modelling a C++ destructor.
class cpp_destructor final : public cpp_function_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builds a [cppast::cpp_destructor]().
    class builder : public basic_builder<cpp_destructor>
    {
    public:
        using basic_builder::basic_builder;

        /// \effects Sets the `virtual`-ness of the destructor.
        void virtual_info(cpp_virtual virt) noexcept
        {
            function->virtual_ = virt;
        }

    private:
        using basic_builder::add_parameter;
        using basic_builder::is_variadic;
    };

    /// \returns Whether or not it is `virtual`.
    bool is_virtual() const noexcept
    {
        return virtual_info().has_value();
    }

    /// \returns The `virtual`-ness of the constructor.
    cpp_virtual virtual_info() const noexcept
    {
        return virtual_;
    }

private:
    cpp_destructor(std::string name) : cpp_function_base(std::move(name)) {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_virtual virtual_;

    friend basic_builder<cpp_destructor>;
};
} // namespace cppast

#endif // CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED
