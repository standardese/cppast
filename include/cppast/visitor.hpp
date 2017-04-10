// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_VISITOR_HPP_INCLUDED
#define CPPAST_VISITOR_HPP_INCLUDED

namespace cppast
{
    class cpp_entity;

    /// Information about the state of a visit operation.
    struct visitor_info
    {
        enum event_type
        {
            leaf_entity, //< Callback called for a leaf entity without children.
            /// If callback returns `false`, visit operation will be aborted.

            container_entity_enter, //< Callback called for a container entity before the children.
            /// If callback returns `false`, none of the children will be visited,
            /// going immediately to the exit event.
            container_entity_exit, //< Callback called for a container entity after the children.
            /// If callback returns `false`, visit operation will be aborted.
        } event;
        bool
            last_child; //< True when the current entity is the last child of the visited parent entity.
        /// \notes It will always be `false` for the initial entity.
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

        bool visit(const cpp_entity& e, visitor_callback_t cb, void* functor, bool last_child);
    } // namespace detail

    /// Visits a [cppast::cpp_entity]().
    /// \effects Invokes the callback for the current entity,
    /// and any child entities.
    /// It will pass a reference to the current entity and the [cppast::visitor_info]().
    /// The return value of the callback controls the visit operation,
    /// the semantic depend on the [cppast::visitor_info::event_type]().
    template <typename Func>
    void visit(const cpp_entity& e, Func f)
    {
        detail::visit(e, &detail::visitor_callback<Func>, &f, false);
    }
} // namespace cppast

#endif // CPPAST_VISITOR_HPP_INCLUDED
