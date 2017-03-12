// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_FUNCTION_HPP_INCLUDED
#define CPPAST_CPP_FUNCTION_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_forward_declarable.hpp>
#include <cppast/cpp_storage_class_specifiers.hpp>
#include <cppast/cpp_variable_base.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling a function parameter.
    class cpp_function_parameter final : public cpp_entity, public cpp_variable_base
    {
    public:
        static cpp_entity_kind kind() noexcept;

        /// \returns A newly created and registered function parameter.
        static std::unique_ptr<cpp_function_parameter> build(
            const cpp_entity_index& idx, cpp_entity_id id, std::string name,
            std::unique_ptr<cpp_type> type, std::unique_ptr<cpp_expression> def = nullptr);

        /// \returns A newly created unnamed function parameter.
        /// \notes It will not be registered, as nothing can refer to it.
        static std::unique_ptr<cpp_function_parameter> build(
            std::unique_ptr<cpp_type> type, std::unique_ptr<cpp_expression> def = nullptr);

    private:
        cpp_function_parameter(std::string name, std::unique_ptr<cpp_type> type,
                               std::unique_ptr<cpp_expression> def)
        : cpp_entity(std::move(name)), cpp_variable_base(std::move(type), std::move(def))
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;
    };

    /// The kinds of function bodies of a [cppast::cpp_function_base]().
    enum cpp_function_body_kind
    {
        cpp_function_declaration, //< Just a declaration.
        cpp_function_definition,  //< Regular definition.
        cpp_function_defaulted,   //< Defaulted definition.
        cpp_function_deleted,     //< Deleted definition.
    };

    /// \returns Whether or not the function body is a declaration,
    /// without a definition.
    inline bool is_declaration(cpp_function_body_kind body) noexcept
    {
        return body == cpp_function_declaration;
    }

    /// \returns Whether or not the function body is a definition.
    inline bool is_definition(cpp_function_body_kind body) noexcept
    {
        return !is_declaration(body);
    }

    /// Base class for all entities that are functions.
    ///
    /// It contains arguments and common flags.
    class cpp_function_base
        : public cpp_entity,
          public cpp_entity_container<cpp_function_base, cpp_function_parameter>,
          public cpp_forward_declarable
    {
    public:
        /// \returns The [cppast::cpp_function_body_kind]().
        /// \notes This matches the [cppast::cpp_forward_declarable]() queries.
        cpp_function_body_kind body_kind() const noexcept
        {
            return body_;
        }

        /// \returns A [ts::optional_ref]() to the [cppast::cpp_expression]() that is the given `noexcept` condition.
        /// \notes If this returns `nullptr`, the function has the implicit noexcept value (i.e. none, unless it is a destructor).
        /// \notes There is no way to distinguish between `noexcept` and `noexcept(true)`.
        type_safe::optional_ref<const cpp_expression> noexcept_condition() const noexcept
        {
            return type_safe::opt_cref(noexcept_expr_.get());
        }

        /// \returns Whether the function has an ellipsis.
        bool is_variadic() const noexcept
        {
            return variadic_;
        }

    protected:
        /// Builder class for functions.
        ///
        /// Inherit from it to provide additional setter.
        template <typename T>
        class basic_builder
        {
        public:
            /// \effects Sets the name.
            basic_builder(std::string name) : function(new T(name))
            {
            }

            /// \effects Adds a parameter.
            void add_parameter(std::unique_ptr<cpp_function_parameter> parameter)
            {
                static_cast<cpp_function_base&>(*function).add_child(std::move(parameter));
            }

            /// \effects Marks the function as variadic.
            void is_variadic()
            {
                static_cast<cpp_function_base&>(*function).variadic_ = true;
            }

            /// \effects Sets the noexcept condition expression.
            void noexcept_condition(std::unique_ptr<cpp_expression> cond)
            {
                static_cast<cpp_function_base&>(*function).noexcept_expr_ = std::move(cond);
            }

            /// \effects If the body is a definition, registers it.
            /// Else marks it as a declaration.
            /// \returns The finished function.
            std::unique_ptr<T> finish(const cpp_entity_index& idx, cpp_entity_id id,
                                      cpp_function_body_kind body_kind)
            {
                function->body_ = body_kind;
                if (cppast::is_definition(body_kind))
                    idx.register_entity(std::move(id), type_safe::cref(*function));
                else
                    function->set_definition(id);
                return std::move(function);
            }

            /// \returns The finished function without registering it.
            /// \notes This is intended for templated functions only.
            std::unique_ptr<T> finish()
            {
                return std::move(function);
            }

        protected:
            basic_builder()           = default;
            ~basic_builder() noexcept = default;

            std::unique_ptr<T> function;
        };

        cpp_function_base(std::string name)
        : cpp_entity(std::move(name)), body_(cpp_function_declaration), variadic_(false)
        {
        }

    private:
        std::unique_ptr<cpp_expression> noexcept_expr_;
        cpp_function_body_kind          body_;
        bool                            variadic_;
    };

    /// A [cppast::cpp_entity]() modelling a C++ function.
    /// \notes This is not a member function,
    /// use [cppast::cpp_member_function]() for that.
    /// It can be a `static` function of a class,
    /// or a `friend`, however.
    class cpp_function final : public cpp_function_base
    {
    public:
        static cpp_entity_kind kind() noexcept;

        /// Builds a [cppast::cpp_function]().
        class builder : public cpp_function_base::basic_builder<cpp_function>
        {
        public:
            /// \effects Sets the name and return type.
            builder(std::string name, std::unique_ptr<cpp_type> return_type)
            {
                function = std::unique_ptr<cpp_function>(
                    new cpp_function(std::move(name), std::move(return_type)));
            }

            /// \effects Sets the storage class.
            void storage_class(cpp_storage_class_specifiers storage)
            {
                function->storage_ = storage;
            }

            /// \effects Marks the function as `constexpr`.
            void is_constexpr()
            {
                function->constexpr_ = true;
            }

            /// \effects Marks the function as `friend`.
            void is_friend()
            {
                function->friend_ = true;
            }
        };

        /// \returns A reference to the return [cppast::cpp_type]().
        const cpp_type& return_type() const noexcept
        {
            return *return_type_;
        }

        /// \returns The [cppast::cpp_storage_specifiers]() of the function.
        /// \notes If it is `cpp_storage_class_static` and inside a [cppast::cpp_class](),
        /// it is a `static` class function.
        cpp_storage_class_specifiers storage_class() const noexcept
        {
            return storage_;
        }

        /// \returns Whether the function is marked `constexpr`.
        bool is_constexpr() const noexcept
        {
            return constexpr_;
        }

        /// \returns Whether the function is a `friend` function of the parent [cppast::cpp_class]().
        bool is_friend() const noexcept
        {
            return friend_;
        }

    private:
        cpp_entity_kind do_get_entity_kind() const noexcept override;

        cpp_function(std::string name, std::unique_ptr<cpp_type> ret)
        : cpp_function_base(std::move(name)),
          return_type_(std::move(ret)),
          storage_(cpp_storage_class_auto),
          friend_(false),
          constexpr_(false)
        {
        }

        std::unique_ptr<cpp_type>    return_type_;
        cpp_storage_class_specifiers storage_;
        bool                         friend_;
        bool                         constexpr_;
    };
} // namespace cppast

#endif // CPPAST_CPP_FUNCTION_HPP_INCLUDED
