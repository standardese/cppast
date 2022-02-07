// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_REF_HPP_INCLUDED

#include <vector>

#include <type_safe/variant.hpp>

#include <cppast/cpp_entity_index.hpp>
#include <cppast/detail/assert.hpp>

namespace cppast
{

/// A basic reference to some kind of [cppast::cpp_entity]().
///
/// It can either refer to a single [cppast::cpp_entity]()
/// or multiple.
/// In the later case it is *overloaded*.
template <typename T, typename Predicate>
class basic_cpp_entity_ref
{
public:
    /// \effects Creates it giving it the target id and name.
    basic_cpp_entity_ref(cpp_entity_id target_id, std::string target_name)
    : target_(std::move(target_id)), name_(std::move(target_name))
    {}

    /// \effects Creates it giving it multiple target ids and name.
    /// \notes This is to refer to an overloaded function.
    basic_cpp_entity_ref(std::vector<cpp_entity_id> target_ids, std::string target_name)
    : target_(std::move(target_ids)), name_(std::move(target_name))
    {}

    /// \returns The name of the reference, as spelled in the source code.
    const std::string& name() const noexcept
    {
        return name_;
    }

    /// \returns Whether or not it refers to multiple entities.
    bool is_overloaded() const noexcept
    {
        return target_.has_value(type_safe::variant_type<std::vector<cpp_entity_id>>{});
    }

    /// \returns The number of entities it refers to.
    type_safe::size_t no_overloaded() const noexcept
    {
        return id().size();
    }

    /// \returns An array reference to the id or ids it refers to.
    type_safe::array_ref<const cpp_entity_id> id() const noexcept
    {
        if (is_overloaded())
        {
            auto& vec = target_.value(type_safe::variant_type<std::vector<cpp_entity_id>>{});
            return type_safe::ref(vec.data(), vec.size());
        }
        else
        {
            auto& id = target_.value(type_safe::variant_type<cpp_entity_id>{});
            return type_safe::ref(&id, 1u);
        }
    }

    /// \returns An array reference to the entities it refers to.
    /// The return type provides `operator[]` + `size()`,
    /// as well as `begin()` and `end()` returning forward iterators.
    /// \exclude return
    std::vector<type_safe::object_ref<const T>> get(const cpp_entity_index& idx) const
    {
        std::vector<type_safe::object_ref<const T>> result;
        get_impl(std::is_convertible<cpp_namespace&, T&>{}, result, idx);
        return result;
    }

private:
    void get_impl(std::true_type, std::vector<type_safe::object_ref<const T>>& result,
                  const cpp_entity_index& idx) const
    {
        for (auto& cur : id())
            for (auto& ns : idx.lookup_namespace(cur))
                result.push_back(ns);
        if (!std::is_same<T, cpp_namespace>::value)
            get_impl(std::false_type{}, result, idx);
    }

    void get_impl(std::false_type, std::vector<type_safe::object_ref<const T>>& result,
                  const cpp_entity_index& idx) const
    {
        for (auto& cur : id())
        {
            auto entity = idx.lookup(cur).map([](const cpp_entity& e) {
                DEBUG_ASSERT(Predicate{}(e), detail::precondition_error_handler{},
                             "invalid entity type");
                return type_safe::ref(static_cast<const T&>(e));
            });
            if (entity)
                result.push_back(type_safe::ref(entity.value()));
        }
    }

    type_safe::variant<cpp_entity_id, std::vector<cpp_entity_id>> target_;
    std::string                                                   name_;
};

/// \exclude
namespace detail
{
    struct cpp_entity_ref_predicate
    {
        bool operator()(const cpp_entity&)
        {
            return true;
        }
    };
} // namespace detail

/// A [cppast::basic_cpp_entity_ref]() to any [cppast::cpp_entity]().
using cpp_entity_ref = basic_cpp_entity_ref<cpp_entity, detail::cpp_entity_ref_predicate>;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
