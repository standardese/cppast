// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_VISITOR_HPP_INCLUDED
#define CPPAST_VISITOR_HPP_INCLUDED

#include <cppast/cpp_class.hpp>
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

        cpp_access_specifier_kind access; //< The current access specifier.

        bool
            last_child; //< True when the current entity is the last child of the visited parent entity.
        /// \notes It will always be `false` for the initial entity.
    };

    /// \exclude
    namespace detail
    {
        using visitor_callback_t  = bool (*)(void* mem, const cpp_entity&, visitor_info info);
        using visitor_predicate_t = bool (*)(const cpp_entity&);

        template <typename Func>
        bool visitor_callback(void* mem, const cpp_entity& e, visitor_info info)
        {
            auto& func = *static_cast<Func*>(mem);
            return func(e, info);
        }

        bool visit(const cpp_entity& e, visitor_callback_t cb, void* functor,
                   cpp_access_specifier_kind cur_access, bool last_child);
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
        detail::visit(e, &detail::visitor_callback<Func>, &f, cpp_public, false);
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
        visit(e, [&](const cpp_entity& e, visitor_info& info) {
            if (pred(e))
                return f(e, info);
            return true;
        });
    }

    /// Generates, at compile-time, a predicate that returns true iff the
    /// given entity holds one of the [cppast::cpp_entity_kind]()s specified
    /// in paramaters of the blacklist template.
    template <cpp_entity_kind... K>
    detail::visitor_predicate_t whitelist()
    {
        static_assert(sizeof...(K) > 0, "At least one entity kind should be specified");
        return [](const cpp_entity& e) {
            bool result = false;
            // this ugliness avoids recursive expansion which would be required in C++11,
            // otherwise known as poor man's fold expression.
            (void)std::initializer_list<int>{(result = result || (K == e.kind()), 0)...};
            return result;
        };
    }

    /// Generates, at compile-time, a predicate that returns true iff the
    /// given entity holds a [cppast::cpp_entity_kind]() that's different
    /// from all of those specified in the paramaters of the blacklist template.
    template <cpp_entity_kind... K>
    detail::visitor_predicate_t blacklist()
    {
        static_assert(sizeof...(K) > 0, "At least one entity kind should be specified");
        return [](const cpp_entity& e) {
            bool result = true;
            (void)std::initializer_list<int>{(result = result && (K != e.kind()), 0)...};
            return result;
        };
    }
} // namespace cppast

#endif // CPPAST_VISITOR_HPP_INCLUDED
