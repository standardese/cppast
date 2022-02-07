// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_TYPE_HPP_INCLUDED
#define CPPAST_CPP_TYPE_HPP_INCLUDED

#include <atomic>
#include <memory>

#include <cppast/code_generator.hpp>
#include <cppast/cpp_entity_ref.hpp>
#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{
/// The kinds of a [cppast::cpp_type]().
enum class cpp_type_kind
{
    builtin_t,
    user_defined_t,

    auto_t,
    decltype_t,
    decltype_auto_t,

    cv_qualified_t,
    pointer_t,
    reference_t,

    array_t,
    function_t,
    member_function_t,
    member_object_t,

    template_parameter_t,
    template_instantiation_t,

    dependent_t,

    unexposed_t,
};

/// Base class for all C++ types.
class cpp_type : detail::intrusive_list_node<cpp_type>
{
public:
    cpp_type(const cpp_type&) = delete;
    cpp_type& operator=(const cpp_type&) = delete;

    virtual ~cpp_type() noexcept = default;

    /// \returns The [cppast::cpp_type_kind]().
    cpp_type_kind kind() const noexcept
    {
        return do_get_kind();
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
    cpp_type() noexcept : user_data_(nullptr) {}

private:
    /// \returns The [cppast::cpp_type_kind]().
    virtual cpp_type_kind do_get_kind() const noexcept = 0;

    void on_insert(const cpp_type&) {}

    mutable std::atomic<void*> user_data_;

    template <typename T>
    friend struct detail::intrusive_list_access;
    friend detail::intrusive_list_node<cpp_type>;
};

/// An unexposed [cppast::cpp_type]().
///
/// This is one where no further information besides a name is available.
class cpp_unexposed_type final : public cpp_type
{
public:
    /// \returns A newly created unexposed type.
    static std::unique_ptr<cpp_unexposed_type> build(std::string name)
    {
        return std::unique_ptr<cpp_unexposed_type>(new cpp_unexposed_type(std::move(name)));
    }

    /// \returns The name of the type.
    const std::string& name() const noexcept
    {
        return name_;
    }

private:
    cpp_unexposed_type(std::string name) : name_(std::move(name)) {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::unexposed_t;
    }

    std::string name_;
};

/// The C++ builtin types.
enum cpp_builtin_type_kind : int
{
    cpp_void, //< `void`

    cpp_bool, //< `bool`

    cpp_uchar,     //< `unsigned char`
    cpp_ushort,    //< `unsigned short`
    cpp_uint,      //< `unsigned int`
    cpp_ulong,     //< `unsigned long`
    cpp_ulonglong, //< `unsigned long long`
    cpp_uint128,   //< `unsigned __int128`

    cpp_schar,    //< `signed char`
    cpp_short,    //< `short`
    cpp_int,      //< `int`
    cpp_long,     //< `long`
    cpp_longlong, //< `long long`
    cpp_int128,   //< `__int128`

    cpp_float,      //< `float`
    cpp_double,     //< `double`
    cpp_longdouble, //< `long double`
    cpp_float128,   //< `__float128`

    cpp_char,   //< `char`
    cpp_wchar,  //< `wchar_t`
    cpp_char16, //< `char16_t`
    cpp_char32, //< `char32_t`

    cpp_nullptr, //< `decltype(nullptr)` aka `std::nullptr_t`
};

/// \returns The string representing the spelling of that type in the source code.
const char* to_string(cpp_builtin_type_kind kind) noexcept;

/// A builtin [cppast::cpp_type]().
///
/// This is one where there is no associated [cppast::cpp_entity]().
class cpp_builtin_type final : public cpp_type
{
public:
    /// \returns A newly created builtin type.
    static std::unique_ptr<cpp_builtin_type> build(cpp_builtin_type_kind kind)
    {
        return std::unique_ptr<cpp_builtin_type>(new cpp_builtin_type(kind));
    }

    /// \returns Which builtin type it is.
    cpp_builtin_type_kind builtin_type_kind() const noexcept
    {
        return kind_;
    }

private:
    cpp_builtin_type(cpp_builtin_type_kind kind) : kind_(kind) {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::builtin_t;
    }

    cpp_builtin_type_kind kind_;
};

/// \exclude
namespace detail
{
    struct cpp_type_ref_predicate
    {
        bool operator()(const cpp_entity& e);
    };
} // namespace detail

/// Reference to a [cppast::cpp_entity]() representing a new type.
using cpp_type_ref = basic_cpp_entity_ref<cpp_entity, detail::cpp_type_ref_predicate>;

/// A user-defined [cppast::cpp_type]().
///
/// It has an associated [cppast::cpp_entity]().
class cpp_user_defined_type final : public cpp_type
{
public:
    /// \returns A newly created user-defined type.
    static std::unique_ptr<cpp_user_defined_type> build(cpp_type_ref entity)
    {
        return std::unique_ptr<cpp_user_defined_type>(new cpp_user_defined_type(std::move(entity)));
    }

    /// \returns A [cppast::cpp_type_ref]() to the associated [cppast::cpp_entity]() that is the
    /// type.
    const cpp_type_ref& entity() const noexcept
    {
        return entity_;
    }

private:
    cpp_user_defined_type(cpp_type_ref entity) : entity_(std::move(entity)) {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::user_defined_t;
    }

    cpp_type_ref entity_;
};

/// A [cppast::cpp_type]() that isn't given but deduced by `auto`.
class cpp_auto_type final : public cpp_type
{
public:
    /// \returns A newly created `auto` type.
    static std::unique_ptr<cpp_auto_type> build()
    {
        return std::unique_ptr<cpp_auto_type>(new cpp_auto_type);
    }

private:
    cpp_auto_type() = default;

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::auto_t;
    }
};

/// A [cppast::cpp_type]() that depends on another type.
class cpp_dependent_type final : public cpp_type
{
public:
    /// \returns A newly created type dependent on a [cppast::cpp_template_parameter_type]().
    static std::unique_ptr<cpp_dependent_type> build(
        std::string name, std::unique_ptr<cpp_template_parameter_type> dependee);

    /// \returns A newly created type dependent on a [cppast::cpp_template_instantiation_type]().
    static std::unique_ptr<cpp_dependent_type> build(
        std::string name, std::unique_ptr<cpp_template_instantiation_type> dependee);

    /// \returns The name of the dependent type.
    /// \notes It does not include a scope.
    const std::string& name() const noexcept
    {
        return name_;
    }

    /// \returns A reference to the [cppast::cpp_type]() it depends one.
    /// \notes This is either [cppast::cpp_template_parameter_type]() or
    /// [cppast:cpp_template_instantiation_type]().
    const cpp_type& dependee() const noexcept
    {
        return *dependee_;
    }

private:
    cpp_dependent_type(std::string name, std::unique_ptr<cpp_type> dependee)
    : name_(std::move(name)), dependee_(std::move(dependee))
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::dependent_t;
    }

    std::string               name_;
    std::unique_ptr<cpp_type> dependee_;
};

/// The kinds of C++ cv qualifiers.
enum cpp_cv : int
{
    cpp_cv_none,
    cpp_cv_const,
    cpp_cv_volatile,
    cpp_cv_const_volatile,
};

/// \returns `true` if the qualifier contains `const`.
inline bool is_const(cpp_cv cv) noexcept
{
    return cv == cpp_cv_const || cv == cpp_cv_const_volatile;
}

/// \returns `true` if the qualifier contains `volatile`.
inline bool is_volatile(cpp_cv cv) noexcept
{
    return cv == cpp_cv_volatile || cv == cpp_cv_const_volatile;
}

/// A [cppast::cpp_cv]() qualified [cppast::cpp_type]().
class cpp_cv_qualified_type final : public cpp_type
{
public:
    /// \returns A newly created qualified type.
    /// \requires `cv` must not be [cppast::cpp_cv::cpp_cv_none]().
    static std::unique_ptr<cpp_cv_qualified_type> build(std::unique_ptr<cpp_type> type, cpp_cv cv)
    {
        DEBUG_ASSERT(cv != cpp_cv_none, detail::precondition_error_handler{});
        return std::unique_ptr<cpp_cv_qualified_type>(
            new cpp_cv_qualified_type(std::move(type), cv));
    }

    /// \returns A reference to the [cppast::cpp_type]() that is qualified.
    const cpp_type& type() const noexcept
    {
        return *type_;
    }

    /// \returns The [cppast::cpp_cv]() qualifier.
    cpp_cv cv_qualifier() const noexcept
    {
        return cv_;
    }

private:
    cpp_cv_qualified_type(std::unique_ptr<cpp_type> type, cpp_cv cv)
    : type_(std::move(type)), cv_(cv)
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::cv_qualified_t;
    }

    std::unique_ptr<cpp_type> type_;
    cpp_cv                    cv_;
};

/// \returns The type without top-level const/volatile qualifiers.
const cpp_type& remove_cv(const cpp_type& type) noexcept;

/// \returns The type without top-level const qualifiers.
const cpp_type& remove_const(const cpp_type& type) noexcept;

/// \returns The type without top-level volatile qualifiers.
const cpp_type& remove_volatile(const cpp_type& type) noexcept;

/// A pointer to a [cppast::cpp_type]().
class cpp_pointer_type final : public cpp_type
{
public:
    /// \returns A newly created pointer type.
    static std::unique_ptr<cpp_pointer_type> build(std::unique_ptr<cpp_type> pointee)
    {
        return std::unique_ptr<cpp_pointer_type>(new cpp_pointer_type(std::move(pointee)));
    }

    /// \returns A reference to the [cppast::cpp_type]() that is the pointee.
    const cpp_type& pointee() const noexcept
    {
        return *pointee_;
    }

private:
    cpp_pointer_type(std::unique_ptr<cpp_type> pointee) : pointee_(std::move(pointee)) {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::pointer_t;
    }

    std::unique_ptr<cpp_type> pointee_;
};

/// The kinds of C++ references.
enum cpp_reference : int
{
    cpp_ref_none,
    cpp_ref_lvalue,
    cpp_ref_rvalue,
};

/// A reference to a [cppast::cpp_type]().
class cpp_reference_type final : public cpp_type
{
public:
    /// \returns A newly created qualified type.
    /// \requires `ref` must not be [cppast::cpp_reference::cpp_ref_none]().
    static std::unique_ptr<cpp_reference_type> build(std::unique_ptr<cpp_type> type,
                                                     cpp_reference             ref)
    {
        DEBUG_ASSERT(ref != cpp_ref_none, detail::precondition_error_handler{});
        return std::unique_ptr<cpp_reference_type>(new cpp_reference_type(std::move(type), ref));
    }

    /// \returns A reference to the [cppast::cpp_type]() that is referenced.
    const cpp_type& referee() const noexcept
    {
        return *referee_;
    }

    /// \returns The [cppast::cpp_reference]() type.
    cpp_reference reference_kind() const noexcept
    {
        return ref_;
    }

private:
    cpp_reference_type(std::unique_ptr<cpp_type> referee, cpp_reference ref)
    : referee_(std::move(referee)), ref_(ref)
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::reference_t;
    }

    std::unique_ptr<cpp_type> referee_;
    cpp_reference             ref_;
};

/// \returns The type as a string representation.
std::string to_string(const cpp_type& type);

/// \exclude
namespace detail
{
    // whether or not it requires special treatment when printing it
    // i.e. function pointer types are complex as the identifier has to be put inside
    bool is_complex_type(const cpp_type& type) noexcept;

    // write part of the type that comes before the variable name
    void write_type_prefix(code_generator::output& output, const cpp_type& type);

    // write part of the type that comes after the name
    // for non-complex types, this does nothing
    void write_type_suffix(code_generator::output& output, const cpp_type& type);

    // write prefix, variadic, name, suffix
    void write_type(code_generator::output& output, const cpp_type& type, std::string name,
                    bool is_variadic = false);
} // namespace detail
} // namespace cppast

#endif // CPPAST_CPP_TYPE_HPP_INCLUDED
