// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_SCOPE_HPP_INCLUDED
#define CPPAST_CPP_SCOPE_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>

namespace cppast
{
    /// Helper class for entities that are containers.
    ///
    /// Inherit from it to generate container access.
    template <class Derived, typename T>
    class cpp_entity_container
    {
    public:
        using iterator = typename detail::intrusive_list<T>::const_iterator;

        /// \returns A const iterator to the first child.
        iterator begin() const noexcept
        {
            return children_.begin();
        }

        /// \returns A const iterator to the last child.
        iterator end() const noexcept
        {
            return children_.end();
        }

    protected:
        /// \effects Adds a new child to the container.
        void add_child(std::unique_ptr<T> ptr) noexcept
        {
            children_.push_back(static_cast<Derived&>(*this), std::move(ptr));
        }

        /// \returns A non-const iterator to the first child.
        typename detail::intrusive_list<T>::iterator mutable_begin() noexcept
        {
            return children_.begin();
        }

        /// \returns A non-const iterator one past the last child.
        typename detail::intrusive_list<T>::iterator mutable_end() noexcept
        {
            return children_.begin();
        }

        ~cpp_entity_container() noexcept = default;

    private:
        detail::intrusive_list<T> children_;
    };

    /// Base class for all entities that add a scope.
    ///
    /// Examples are namespaces and classes,
    /// or anything else that can appear followed by `::`.
    class cpp_scope : public cpp_entity, public cpp_entity_container<cpp_scope, cpp_entity>
    {
    public:
        /// \returns The name of the scope.
        /// By default, this is the same name as the entity,
        /// but derived classes can override it.
        std::string scope_name() const
        {
            return do_get_scope_name();
        }

    protected:
        using cpp_entity::cpp_entity;

    private:
        /// \returns The name of the new scope,
        /// defaults to the name of the entity.
        virtual std::string do_get_scope_name() const
        {
            return name();
        }
    };
} // namespace cppast

#endif // CPPAST_CPP_SCOPE_HPP_INCLUDED
