// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_VISITOR_HPP_INCLUDED
#define CPPAST_VISITOR_HPP_INCLUDED

namespace cppast
{
    class cpp_entity;

    /// Information about the state of a visit operation.
    enum class visitor_info
    {
        leave_entity, //< Callback called for a leave entity without children.

        container_entity_enter, //< Callback called for a container entity before the children.
        container_entity_exit,  //< Callback called for a container entity after the children.
    };

    /// \exclude
    namespace detail
    {
        using visitor_callback_t = bool (*)(void* mem, const cpp_entity&, visitor_info info);

        template <typename Func>
        bool visitor_callback(void* mem, const cpp_entity& e, visitor_info info)
        {
            auto& func = *static_cast<Func*>(mem);
            return func(e, info);
        }

        bool visit(const cpp_entity& e, visitor_callback_t cb, void* functor);
    } // namespace detail

    /// Visits a [cppast::cpp_entity]().
    /// \effects If the given entity is a container, i.e. if it has child entities,
    /// calls `f(e, visitor_info::container_entity_enter)`.
    /// If that returns `true`, recursively calls `visit()` for all child entities,
    /// followed by a call to `f(e, visitor_info::container_entity_exit)`.
    /// If the given entity is not a container, calls `f(e, visitor_info::leave_entity)`.
    /// If the functor returns `false` for [cppast::visitor_info]() other than `container_entity_enter`,
    /// the visit operation is aborted.
    template <typename Func>
    void visit(const cpp_entity& e, Func f)
    {
        detail::visit(e, &detail::visitor_callback<Func>, &f);
    }
} // namespace cppast

#endif // CPPAST_VISITOR_HPP_INCLUDED
