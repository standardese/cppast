// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED

#include <unordered_map>
#include <mutex>
#include <string>

#include <type_safe/optional_ref.hpp>
#include <type_safe/reference.hpp>
#include <type_safe/strong_typedef.hpp>

#include <cppast/detail/assert.hpp>

namespace cppast
{
    class cpp_entity;

    /// \exclude
    namespace detail
    {
        constexpr std::size_t fnv_basis = 14695981039346656037ull;
        constexpr std::size_t fnv_prime = 1099511628211ull;

        // FNV-1a 64 bit hash
        constexpr std::size_t id_hash(const char* str, std::size_t hash = fnv_basis)
        {
            return *str ? id_hash(str + 1, (hash ^ *str) * fnv_prime) : hash;
        }
    } // namespace detail

    /// A [ts::strong_typedef]() representing the unique id of a [cppast::cpp_entity]().
    ///
    /// It is comparable for equality.
    struct cpp_entity_id : type_safe::strong_typedef<cpp_entity_id, std::size_t>,
                           type_safe::strong_typedef_op::equality_comparison<cpp_entity_id, bool>
    {
        explicit cpp_entity_id(const std::string& str) : cpp_entity_id(str.c_str())
        {
        }

        explicit cpp_entity_id(const char* str) : strong_typedef(detail::id_hash(str))
        {
        }
    };

    inline namespace literals
    {
        /// \returns A new [cppast::cpp_entity_id]() created from the given string.
        inline cpp_entity_id operator""_id(const char* str, std::size_t)
        {
            return cpp_entity_id(str);
        }
    }

    /// An index of all [cppast::cpp_entity]() objects created.
    ///
    /// It maps [cppast::cpp_entity_id]() to references to the [cppast::cpp_entity]() objects.
    class cpp_entity_index
    {
    public:
        /// \effects Registers a new [cppast::cpp_entity]() also giving its [cppast::cpp_entity_id]().
        /// \requires The entity must not have been registered before,
        /// and it must live as long as the index lives.
        /// \notes This operation is thread safe.
        void register_entity(cpp_entity_id id, type_safe::object_ref<const cpp_entity> entity) const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto                        result = map_.emplace(std::move(id), std::move(entity));
            DEBUG_ASSERT(result.second, detail::precondition_error_handler{},
                         "duplicate index registration");
        }

        /// \returns A [ts::optional_ref]() corresponding to the entity of the given [cppast::cpp_entity_id]().
        /// \notes This operation is thread safe.
        type_safe::optional_ref<const cpp_entity> lookup(const cpp_entity_id& id) const noexcept
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto                        iter = map_.find(id);
            if (iter == map_.end())
                return {};
            return iter->second.get();
        }

    private:
        struct hash
        {
            std::size_t operator()(const cpp_entity_id& id) const noexcept
            {
                return static_cast<std::size_t>(id);
            }
        };

        mutable std::mutex mutex_;
        mutable std::unordered_map<cpp_entity_id, type_safe::object_ref<const cpp_entity>, hash>
            map_;
    };
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_INDEX_HPP_INCLUDED
