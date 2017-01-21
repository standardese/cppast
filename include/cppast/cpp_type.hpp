// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_TYPE_HPP_INCLUDED
#define CPPAST_CPP_TYPE_HPP_INCLUDED

#include <memory>

#include <cppast/detail/intrusive_list.hpp>
#include <cppast/cpp_entity_ref.hpp>

namespace cppast
{
    /// The kinds of a [cppast::cpp_type]().
    enum class cpp_type_kind
    {
        builtin,
        user_defined,

        cv_qualified,
        pointer,
        reference,

        unexposed,
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

    protected:
        cpp_type() noexcept = default;

    private:
        /// \returns The [cppast::cpp_type_kind]().
        virtual cpp_type_kind do_get_kind() const noexcept = 0;

        void on_insert(const cpp_type&)
        {
        }

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
        std::unique_ptr<cpp_unexposed_type> build(std::string name)
        {
            return std::unique_ptr<cpp_unexposed_type>(new cpp_unexposed_type(std::move(name)));
        }

        /// \returns The name of the type.
        const std::string& name() const noexcept
        {
            return name_;
        }

    private:
        cpp_unexposed_type(std::string name) : name_(std::move(name))
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::unexposed;
        }

        std::string name_;
    };

    /// A builtin [cppast::cpp_type]().
    ///
    /// This is one where there is no associated [cppast::cpp_entity]().
    class cpp_builtin_type final : public cpp_type
    {
    public:
        /// \returns A newly created builtin type.
        std::unique_ptr<cpp_builtin_type> build(std::string name)
        {
            return std::unique_ptr<cpp_builtin_type>(new cpp_builtin_type(std::move(name)));
        }

        /// \returns The name of the type.
        const std::string& name() const noexcept
        {
            return name_;
        }

    private:
        cpp_builtin_type(std::string name) : name_(std::move(name))
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::builtin;
        }

        std::string name_;
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
        std::unique_ptr<cpp_user_defined_type> build(cpp_type_ref entity)
        {
            return std::unique_ptr<cpp_user_defined_type>(
                new cpp_user_defined_type(std::move(entity)));
        }

        /// \returns A [cppast::cpp_type_ref]() to the associated [cppast::cpp_entity]() that is the type.
        const cpp_type_ref& entity() const noexcept
        {
            return entity_;
        }

    private:
        cpp_user_defined_type(cpp_type_ref entity) : entity_(std::move(entity))
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::user_defined;
        }

        cpp_type_ref entity_;
    };

    /// The kinds of C++ cv qualifiers.
    enum cpp_cv
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
        static std::unique_ptr<cpp_cv_qualified_type> build(std::unique_ptr<cpp_type> type,
                                                            cpp_cv                    cv)
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
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::cv_qualified;
        }

        std::unique_ptr<cpp_type> type_;
        cpp_cv                    cv_;
    };

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
        cpp_pointer_type(std::unique_ptr<cpp_type> pointee) : pointee_(std::move(pointee))
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::pointer;
        }

        std::unique_ptr<cpp_type> pointee_;
    };

    /// The kinds of C++ references.
    enum cpp_reference
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
            return std::unique_ptr<cpp_reference_type>(
                new cpp_reference_type(std::move(type), ref));
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
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::reference;
        }

        std::unique_ptr<cpp_type> referee_;
        cpp_reference             ref_;
    };
} // namespace cppast

#endif // CPPAST_CPP_TYPE_HPP_INCLUDED
