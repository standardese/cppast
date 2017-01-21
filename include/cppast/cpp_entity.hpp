// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{
    enum class cpp_entity_type;

    /// The base class for all entities in the C++ AST.
    class cpp_entity : detail::intrusive_list_node<cpp_entity>
    {
    public:
        cpp_entity(const cpp_entity&) = delete;
        cpp_entity& operator=(const cpp_entity&) = delete;

        virtual ~cpp_entity() noexcept = default;

        /// \returns The type of the entity.
        cpp_entity_type type() const noexcept
        {
            return do_get_entity_type();
        }

        /// \returns The name of the entity.
        /// The name is the string associated with the entity's declaration.
        const std::string& name() const noexcept
        {
            return name_;
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
        /// \returns The type of the entity.
        virtual cpp_entity_type do_get_entity_type() const noexcept = 0;

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
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_HPP_INCLUDED
