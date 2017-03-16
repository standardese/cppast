// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENUM_HPP_INCLUDED
#define CPPAST_CPP_ENUM_HPP_INCLUDED

#include <memory>

#include <type_safe/optional_ref.hpp>

#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_forward_declarable.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling the value of an [cppast::cpp_enum]().
    class cpp_enum_value final : public cpp_entity
    {
    public:
        static cpp_entity_kind kind() noexcept;

        /// \returns A newly created and registered enum value.
        /// \notes `value` may be `nullptr`, in which case the enum has an implicit value.
        static std::unique_ptr<cpp_enum_value> build(
            const cpp_entity_index& idx, cpp_entity_id id, std::string name,
            std::unique_ptr<cpp_expression> value = nullptr);

        /// \returns A [ts::optional_ref]() to the [cppast::cpp_expression]() that is the enum value.
        /// \notes It only has an associated expression if the value is explictly given.
        type_safe::optional_ref<const cpp_expression> value() const noexcept
        {
            return type_safe::opt_cref(value_.get());
        }

    private:
        cpp_enum_value(std::string name, std::unique_ptr<cpp_expression> value)
        : cpp_entity(std::move(name)), value_(std::move(value))
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        std::unique_ptr<cpp_expression> value_;
    };

    /// A [cppast::cpp_entity]() modelling a C++ enumeration.
    ///
    /// This can either be a definition or just a forward declaration.
    /// If it is just forward declared, it will not have any children.
    class cpp_enum final : public cpp_entity,
                           public cpp_entity_container<cpp_enum, cpp_enum_value>,
                           public cpp_forward_declarable
    {
    public:
        static cpp_entity_kind kind() noexcept;

        /// Builds a [cppast::cpp_enum]().
        class builder
        {
        public:
            /// \effects Sets the name, underlying type and whether it is scoped.
            /// \notes The underlying type may be `nullptr` if it is not explictly given.
            builder(std::string name, bool scoped, std::unique_ptr<cpp_type> type = nullptr)
            : enum_(new cpp_enum(std::move(name), std::move(type), scoped))
            {
            }

            /// \effects Adds a [cppast::cpp_enum_value]().
            void add_value(std::unique_ptr<cpp_enum_value> value)
            {
                enum_->add_child(std::move(value));
            }

            /// \returns The not yet finished enumeration.
            cpp_enum& get() noexcept
            {
                return *enum_;
            }

            /// \effects Registers the enum in the [cppast::cpp_entity_index](),
            /// using the given [cppast::cpp_entity_id]().
            /// \returns The finished enum.
            std::unique_ptr<cpp_enum> finish(const cpp_entity_index& idx, cpp_entity_id id) noexcept
            {
                idx.register_entity(std::move(id), type_safe::ref(*enum_));
                return std::move(enum_);
            }

            /// \effects Marks the enum as forward declaration.
            /// \returns The finished enum.
            /// \notes It will not be registered, as it is not the main definition.
            std::unique_ptr<cpp_enum> finish_declaration(cpp_entity_id definition_id) noexcept
            {
                enum_->set_definition(definition_id);
                return std::move(enum_);
            }

        private:
            std::unique_ptr<cpp_enum> enum_;
        };

        /// \returns A [ts::optional_ref]() to the underlying [cppast::cpp_type]() of the enum.
        /// \notes It only has an associated underlying type if it is not explictly given.
        type_safe::optional_ref<const cpp_type> underlying_type() const noexcept
        {
            return type_safe::opt_cref(type_.get());
        }

        /// \returns Whether or not it is a scoped enumeration (i.e. an `enum class`).
        bool is_scoped() const noexcept
        {
            return scoped_;
        }

    private:
        cpp_enum(std::string name, std::unique_ptr<cpp_type> type, bool scoped)
        : cpp_entity(std::move(name)), type_(std::move(type)), scoped_(scoped)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        /// \returns If the enum is scoped, the name of the enum,
        /// otherwise [ts::nullopt]().
        type_safe::optional<std::string> do_get_scope_name() const override;

        std::unique_ptr<cpp_type> type_;
        bool                      scoped_;
    };
} // namespace cppast

#endif // CPPAST_CPP_ENUM_HPP_INCLUDED
