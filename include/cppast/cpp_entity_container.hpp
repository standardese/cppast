// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_ENTITY_CONTAINER_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_CONTAINER_HPP_INCLUDED

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
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_CONTAINER_HPP_INCLUDED
