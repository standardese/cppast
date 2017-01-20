// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_SCOPE_HPP_INCLUDED
#define CPPAST_CPP_SCOPE_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>

namespace cppast
{
    /// Base class for all entities that add a scope.
    ///
    /// Examples are namespaces and classes,
    /// or anything else that can appear followed by `::`.
    class cpp_scope : public cpp_entity
    {
    public:
        /// \returns The name of the scope.
        /// By default, this is the same name as the entity,
        /// but derived classes can override it.
        std::string scope_name() const
        {
            return do_get_scope_name();
        }

        using iterator = detail::intrusive_list<cpp_entity>::const_iterator;

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
        using cpp_entity::cpp_entity;

        /// \effects Adds a new child to the scope.
        void add_child(std::unique_ptr<cpp_entity> ptr) noexcept
        {
            DEBUG_ASSERT(ptr->parent() == *this, detail::precondition_error_handler{},
                         "parent not set properly");
            children_.push_back(std::move(ptr));
        }

        /// \returns A non-const iterator to the first child.
        detail::intrusive_list<cpp_entity>::iterator mutable_begin() noexcept
        {
            return children_.begin();
        }

        /// \returns A non-const iterator one past the last child.
        detail::intrusive_list<cpp_entity>::iterator mutable_end() noexcept
        {
            return children_.begin();
        }

    private:
        /// \returns The name of the new scope,
        /// defaults to the name of the entity.
        virtual std::string do_get_scope_name() const
        {
            return name();
        }

        detail::intrusive_list<cpp_entity> children_;
    };
} // namespace cppast

#endif // CPPAST_CPP_SCOPE_HPP_INCLUDED
