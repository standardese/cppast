// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_FRIEND_HPP_INCLUDED
#define CPPAST_CPP_FRIEND_HPP_INCLUDED

#include <type_safe/optional.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() representing a friend declaration.
///
/// It can either declare or define a `friend` function (template), declare a `friend` class,
/// or refer to an existing type.
class cpp_friend : public cpp_entity, private cpp_entity_container<cpp_friend, cpp_entity>
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created friend declaring the given entity as `friend`.
    /// \notes The friend declaration itself will not be registered,
    /// but the referring entity is.
    static std::unique_ptr<cpp_friend> build(std::unique_ptr<cpp_entity> e)
    {
        return std::unique_ptr<cpp_friend>(new cpp_friend(std::move(e)));
    }

    /// \returns A newly created friend declaring the given type as `friend`.
    /// \notes It will not be registered.
    static std::unique_ptr<cpp_friend> build(std::unique_ptr<cpp_type> type)
    {
        return std::unique_ptr<cpp_friend>(new cpp_friend(std::move(type)));
    }

    /// \returns An optional reference to the entity it declares as friend, or `nullptr`.
    type_safe::optional_ref<const cpp_entity> entity() const noexcept
    {
        if (begin() == end())
            return nullptr;
        return type_safe::ref(*begin());
    }

    /// \returns An optional reference to the type it declares as friend, or `nullptr`.
    type_safe::optional_ref<const cpp_type> type() const noexcept
    {
        return type_safe::opt_ref(type_.get());
    }

private:
    cpp_friend(std::unique_ptr<cpp_entity> e) : cpp_entity("")
    {
        add_child(std::move(e));
    }

    cpp_friend(std::unique_ptr<cpp_type> type) : cpp_entity(""), type_(std::move(type)) {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    std::unique_ptr<cpp_type> type_;

    friend cpp_entity_container<cpp_friend, cpp_entity>;
};
} // namespace cppast

#endif // CPPAST_CPP_FRIEND_HPP_INCLUDED
