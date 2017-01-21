// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_REF_HPP_INCLUDED

#include <cppast/cpp_entity_index.hpp>

namespace cppast
{
    /// A basic reference to some kind of [cppast::cpp_entity]().
    template <typename T, typename Predicate>
    class basic_cpp_entity_ref
    {
    public:
        /// \effects Creates it giving it the target id and name.
        /// \requires An entity of matching type must eventually register in the [cppast::cpp_entity_index]() using that id.
        basic_cpp_entity_ref(cpp_entity_id target_id, std::string target_name)
        : target_(std::move(target_id)), name_(std::move(target_name))
        {
        }

        /// \returns The name of the reference, as spelled in the source code.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns The [cppast::cpp_entity_id]() of the entity it refers to.
        const cpp_entity_id& id() const noexcept
        {
            return target_;
        }

        /// \returns The [cppast::cpp_entity]() it refers to.
        const T& get(const cpp_entity_index& idx) const noexcept
        {
            auto entity = idx.lookup(target_);
            DEBUG_ASSERT(Predicate{}(entity.value()), detail::precondition_error_handler{},
                         "invalid entity");
            return static_cast<const T&>(entity.value());
        }

    private:
        cpp_entity_id target_;
        std::string   name_;
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
    }

    /// A [cppast::basic_cpp_entity_ref]() to any [cppast::cpp_entity]().
    using cpp_entity_ref = basic_cpp_entity_ref<cpp_entity, detail::cpp_entity_ref_predicate>;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
