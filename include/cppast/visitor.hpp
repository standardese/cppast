// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_VISITOR_HPP_INCLUDED
#define CPPAST_VISITOR_HPP_INCLUDED

#include <type_traits>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>

namespace cppast
{
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
        using visitor_filter_t   = bool (*)(void* mem, const cpp_entity&);

        template <typename Func>
        bool visitor_callback(void* mem, const cpp_entity& e, visitor_info info)
        {
            auto& func = *static_cast<Func*>(mem);
            return func(e, info);
        }

        template <typename Predicate>
        bool visitor_filter_callback(void *mem, const cpp_entity &e)
        {
            auto& func = *static_cast<Predicate*>(mem);
            return func(e);
        }

        // the filtered visitor overload accepts any functor of a similar signature,
        // not just this one. This type is for the whitelist and blacklist utility functions.
        using visitor_predicate_t = bool (*)(const cpp_entity&);

        bool visit(const cpp_entity& e, visitor_callback_t cb, void* functor, bool last_child);
        bool visit(const cpp_entity& e, visitor_filter_t fcb, void* filt_functor,
                   visitor_callback_t cb, void* vis_functor, bool last_child);
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

    /// Visits a [cppast::cpp_entity]().
    /// \effects Invokes the callback for the current entity,
    /// and child entities that match the given predicate.
    /// It will pass a reference to the current entity and the [cppast::visitor_info]().
    /// The return value of the callback controls the visit operation,
    /// the semantic depend on the [cppast::visitor_info::event_type]().
    template <typename Func, typename Predicate>
    void visit(const cpp_entity& e, Predicate pred, Func f)
    {
        detail::visit(e, &detail::visitor_filter_callback<Predicate>, &pred,
                      &detail::visitor_callback<Func>, &f, false);
    }


    template<cpp_entity_kind... Kinds>
    detail::visitor_predicate_t whitelist()
    {
        return [](const cpp_entity& e)
        {
            static constexpr std::array<cpp_entity_kind, sizeof...(Kinds)> kinds_arr { Kinds... };
            for (auto& k : kinds_arr)
                if (k == e.kind())
                    return true;
            return false;
        };
    }

} // namespace cppast

#endif // CPPAST_VISITOR_HPP_INCLUDED
