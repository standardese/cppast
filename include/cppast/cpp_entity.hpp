// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{
    enum class cpp_entity_kind;

    /// The base class for all entities in the C++ AST.
    class cpp_entity : detail::intrusive_list_node<cpp_entity>
    {
    public:
        cpp_entity(const cpp_entity&) = delete;
        cpp_entity& operator=(const cpp_entity&) = delete;

        virtual ~cpp_entity() noexcept = default;

        /// \returns The kind of the entity.
        cpp_entity_kind kind() const noexcept
        {
            return do_get_entity_kind();
        }

        /// \returns The name of the entity.
        /// The name is the string associated with the entity's declaration.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns The name of the new scope created by the entity,
        /// if there is any.
        type_safe::optional<std::string> scope_name() const
        {
            return do_get_scope_name();
        }

        /// \returns A [ts::optional_ref]() to the parent entity in the AST.
        type_safe::optional_ref<const cpp_entity> parent() const noexcept
        {
            return parent_;
        }

    protected:
        /// \effects Creates it giving it the the name.
        cpp_entity(std::string name) : name_(std::move(name))
        {
        }

    private:
        /// \returns The kind of the entity.
        virtual cpp_entity_kind do_get_entity_kind() const noexcept = 0;

        /// \returns The name of the new scope created by the entity, if any.
        /// By default, there is no scope created.
        virtual type_safe::optional<std::string> do_get_scope_name() const
        {
            return type_safe::nullopt;
        }

        void on_insert(const cpp_entity& parent) noexcept
        {
            parent_ = parent;
        }

        type_safe::optional_ref<const cpp_entity> parent_;
        std::string                               name_;

        template <typename T>
        friend struct detail::intrusive_list_access;
        friend detail::intrusive_list_node<cpp_entity>;
    };

    /// \returns The full name of the [cppast::cpp_entity](), with all scopes.
    std::string full_name(const cpp_entity& e);
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_HPP_INCLUDED
