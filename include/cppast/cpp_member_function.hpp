// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED
#define CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED

#include <cppast/cpp_function.hpp>

namespace cppast
{
    /// The `virtual`-ness of a member function.
    enum cpp_virtual
    {
        cpp_virtual_none,     //< Not `virtual`.
        cpp_virtual_pure,     //< Pure `virtual` function.
        cpp_virtual_new,      //< New `virtual` function.
        cpp_virtual_override, //< Overriden `virtual` function (attribute doesn't matter).
        cpp_virtual_final,    //< `final` `virtual` function.
    };

    /// \returns Whether or not the given flag means the function is `virtual`.
    inline bool is_virtual(cpp_virtual virt) noexcept
    {
        return virt != cpp_virtual_none;
    }

    /// \returns Whether or not the given flag means the function overrides a `virtual` function.
    inline bool is_overriden(cpp_virtual virt) noexcept
    {
        return virt == cpp_virtual_override || virt == cpp_virtual_final;
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

        /// \returns The `virtual`-ness of the member function.
        cpp_virtual virtual_info() const noexcept
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
            void virtual_info(cpp_virtual virt) noexcept
            {
                static_cast<cpp_member_function_base&>(*this->function).virtual_ = virt;
            }

            /// \effects Marks the function as `constexpr`.
            void is_constexpr() noexcept
            {
                static_cast<cpp_member_function_base&>(*this->function).constexpr_ = true;
            }
        };

        /// \effects Sets name and return type, as well as the rest to defaults.
        cpp_member_function_base(std::string name, std::unique_ptr<cpp_type> return_type)
        : cpp_function_base(std::move(name)),
          return_type_(std::move(return_type)),
          virtual_(cpp_virtual_none),
          cv_(cpp_cv_none),
          ref_(cpp_ref_none),
          constexpr_(false)
        {
        }

    private:
        std::unique_ptr<cpp_type> return_type_;
        cpp_virtual               virtual_;
        cpp_cv                    cv_;
        cpp_reference             ref_;
        bool                      constexpr_;
    };

    /// A [cppast::cpp_entity]() modelling a member function.
    class cpp_member_function final : public cpp_member_function_base
    {
    public:
        /// Builder for [cppast::cpp_member_function]().
        class builder : public basic_member_builder<cpp_member_function>
        {
        public:
            using basic_member_builder::basic_member_builder;
        };

    private:
        using cpp_member_function_base::cpp_member_function_base;

        cpp_entity_kind do_get_entity_kind() const noexcept override;
    };

    /// A [cppast::cpp_entity]() modelling a C++ conversion operator.
    class cpp_conversion_op final : public cpp_member_function_base
    {
    public:
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
        };

        /// \returns Whether or not the conversion is `explicit`.
        bool is_explicit() const noexcept
        {
            return explicit_;
        }

    private:
        cpp_conversion_op(std::string name, std::unique_ptr<cpp_type> return_t)
        : cpp_member_function_base(std::move(name), std::move(return_t)), explicit_(false)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        bool explicit_;
    };

    /// A [cppast::cpp_entity]() modelling a C++ constructor.
    class cpp_constructor final : public cpp_function_base
    {
    public:
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

    private:
        cpp_constructor(std::string name)
        : cpp_function_base(std::move(name)), explicit_(false), constexpr_(false)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        bool explicit_;
        bool constexpr_;
    };

    /// A [cppast::cpp_entity]() modelling a C++ destructor.
    class cpp_destructor final : public cpp_function_base
    {
    public:
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

        /// \returns The `virtual`-ness of the constructor.
        cpp_virtual virtual_info() const noexcept
        {
            return virtual_;
        }

    private:
        cpp_destructor(std::string name)
        : cpp_function_base(std::move(name)), virtual_(cpp_virtual_none)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        cpp_virtual virtual_;
    };
} // namespace cppast

#endif // CPPAST_CPP_MEMBER_FUNCTION_HPP_INCLUDED
