// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_VISITOR_HPP_INCLUDED
#define CPPAST_VISITOR_HPP_INCLUDED

#include <type_traits>

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

    /// True when the current entity is the last child of the visited parent entity.
    /// \notes It will always be `false` for the initial entity.
    bool last_child;

    /// \returns `true` if the entity was not visited already.
    bool is_new_entity() const noexcept
    {
        return event != container_entity_exit;
    }

    /// \returns `true` if the entity was visited already.
    bool is_old_entity() const noexcept
    {
        return !is_new_entity();
    }
};

/// A more expressive way to specify the return of a visit operation.
enum visitor_result : bool
{
    /// Visit should continue.
    /// \group continue
    continue_visit = true,

    /// \group continue
    continue_visit_children = true,

    /// Visit should not visit the children.
    /// \notes This only happens when the event is [cppast::visitor_info::container_entity_enter]().
    continue_visit_no_children = false,

    /// Visit should be aborted.
    /// \notes This only happens when the event is not
    /// [cppast::visitor_info::container_entity_enter]().
    abort_visit = false,
};

/// \exclude
namespace detail
{
    using visitor_callback_t = bool (*)(void* mem, const cpp_entity&, visitor_info info);

    struct visitor_info_void
    {};
    struct visitor_info_bool : visitor_info_void
    {};

    template <typename Func>
    visitor_callback_t get_visitor_callback(
        visitor_info_bool,
        decltype(std::declval<Func>()(std::declval<const cpp_entity&>(), visitor_info{})) = true)
    {
        return [](void* mem, const cpp_entity& e, visitor_info info) {
            auto& func = *static_cast<Func*>(mem);
            return func(e, info);
        };
    }

    template <typename Func>
    visitor_callback_t get_visitor_callback(
        visitor_info_void,
        decltype(std::declval<Func>()(std::declval<const cpp_entity&>(), visitor_info{}), 0) = 0)
    {
        return [](void* mem, const cpp_entity& e, visitor_info info) {
            auto& func = *static_cast<Func*>(mem);
            func(e, info);
            return true;
        };
    }

    template <typename Func>
    visitor_callback_t get_visitor_callback()
    {
        return get_visitor_callback<Func>(visitor_info_bool{});
    }

    bool visit(const cpp_entity& e, visitor_callback_t cb, void* functor,
               cpp_access_specifier_kind cur_access, bool last_child);
} // namespace detail

/// Visits a [cppast::cpp_entity]() and children.
///
/// \effects It will invoke the callback - the visitor - for the current entity and children,
/// as controlled by the [cppast::visitor_result]().
/// The visitor is given a reference to the currently visited entity and the
/// [cppast::visitor_info]().
///
/// \requires The visitor must be callable as specified and must either return `bool` or nothing.
/// If it returns nothing, a return value [cppast::visitor_result::continue_visit]() is assumed.
template <typename Func>
void visit(const cpp_entity& e, Func f)
{
    detail::visit(e, detail::get_visitor_callback<Func>(), &f, cpp_public, false);
}

/// The result of a visitor filter operation.
enum class visit_filter
{
    include = true,  //< The entity is included.
    exclude = false, //< The entity is excluded and will not be visited.
    exclude_and_children
    = 2, //< The entity and all direct children are excluded and will not be visited.
};

namespace detail
{
    using visitor_filter_t = visit_filter (*)(const cpp_entity&);

    template <typename Filter>
    auto invoke_visit_filter(int, Filter f, const cpp_entity& e, cpp_access_specifier_kind access)
        -> decltype(static_cast<visit_filter>(f(e, access)))
    {
        return static_cast<visit_filter>(f(e, access));
    }

    template <typename Filter>
    auto invoke_visit_filter(short, Filter f, const cpp_entity& e, cpp_access_specifier_kind)
        -> decltype(static_cast<visit_filter>(f(e)))
    {
        return static_cast<visit_filter>(f(e));
    }
} // namespace detail

/// Visits a [cppast::cpp_entity]() and children that pass a given filter.
///
/// \effects It behaves like the non-filtered visit except it will only invoke the visitor for
/// entities that match the filter. The filter is a predicate that will be invoked with the current
/// entity and access specifier first and the result converted to [cppast::visit_filter](). Visit
/// will behave accordingly.
///
/// \requires The visitor must be as specified for the other overload.
/// The filter must be a function taking a [cppast::cpp_entity]() as first parameter and optionally
/// a [cppast::cpp_access_specifier_kind]() as second. It must return a `bool` or a
/// [cppast::visit_filter]().
template <typename Func, typename Filter>
void visit(const cpp_entity& e, Filter filter, Func f)
{
    visit(e, [&](const cpp_entity& e, const visitor_info& info) -> bool {
        auto result = detail::invoke_visit_filter(0, filter, e, info.access);
        switch (result)
        {
        case visit_filter::include:
            return detail::get_visitor_callback<Func>()(&f, e, info);
        case visit_filter::exclude:
            return continue_visit;
        case visit_filter::exclude_and_children:
            // exclude children if entering
            if (info.event == visitor_info::container_entity_enter)
                return continue_visit_no_children;
            else
                return continue_visit;
        }
        return continue_visit;
    });
}

/// \exclude
namespace detail
{
    template <cpp_entity_kind... K>
    bool has_one_of_kind(const cpp_entity& e)
    {
        static_assert(sizeof...(K) > 0, "At least one entity kind must be specified");
        bool result = false;

        // poor men's fold
        int dummy[]{(result |= (K == e.kind()), 0)...};
        (void)dummy;

        return result;
    }
} // namespace detail

/// Generates a blacklist visitor filter.
///
/// \returns A visitor filter that excludes all entities having one of the specified kinds.
template <cpp_entity_kind... Kinds>
detail::visitor_filter_t blacklist()
{
    return [](const cpp_entity& e) {
        return detail::has_one_of_kind<Kinds...>(e) ? visit_filter::exclude : visit_filter::include;
    };
}

/// Generates a blacklist visitor filter.
///
/// \returns A visitor filter that excludes all entities having one of the specified kinds and
/// children of those entities.
template <cpp_entity_kind... Kinds>
detail::visitor_filter_t blacklist_and_children()
{
    return [](const cpp_entity& e) {
        return detail::has_one_of_kind<Kinds...>(e) ? visit_filter::exclude_and_children
                                                    : visit_filter::include;
    };
}

/// Generates a whitelist visitor filter.
///
/// \returns A visitor filter that excludes all entities having not one of the specified kinds.
template <cpp_entity_kind... Kinds>
detail::visitor_filter_t whitelist()
{
    return [](const cpp_entity& e) {
        return detail::has_one_of_kind<Kinds...>(e) ? visit_filter::include : visit_filter::exclude;
    };
}
} // namespace cppast

#endif // CPPAST_VISITOR_HPP_INCLUDED
