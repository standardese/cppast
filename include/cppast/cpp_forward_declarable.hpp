// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_FORWARD_DECLARABLE_HPP_INCLUDED
#define CPPAST_CPP_FORWARD_DECLARABLE_HPP_INCLUDED

#include <type_traits>

#include <type_safe/optional.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_ref.hpp>

namespace cppast
{
    /// Mixin base class for all entities that can have a forward declaration.
    ///
    /// Examples are [cppast::cpp_enum]() or [cppast::cpp_class](),
    /// but also [cppast::cpp_function_base]().
    /// Those entities can have multiple declarations and one definition.
    class cpp_forward_declarable
    {
    public:
        /// \returns Whether or not the entity is the definition.
        bool is_definition() const noexcept
        {
            return !definition_.has_value();
        }

        /// \returns Whether or not the entity is "just" a declaration.
        bool is_declaration() const noexcept
        {
            return definition_.has_value();
        }

        /// \returns The [cppast::cpp_entity_id]() of the definition,
        /// if the current entity is not the definition.
        const type_safe::optional<cpp_entity_id>& definition() const noexcept
        {
            return definition_;
        }

    protected:
        /// \effects Marks the entity as definition.
        /// \notes If it is not a definition,
        /// [*set_definition]() must be called.
        cpp_forward_declarable() noexcept = default;

        ~cpp_forward_declarable() noexcept = default;

        /// \effects Sets the definition of the entity,
        /// marking it as a forward declaration.
        void set_definition(cpp_entity_id def) noexcept
        {
            definition_ = std::move(def);
        }

    private:
        type_safe::optional<cpp_entity_id> definition_;
    };

    /// \exclude
    namespace detail
    {
        template <typename T>
        auto get_definition_impl(const cpp_entity_index& idx, const T& entity) ->
            typename std::enable_if<std::is_base_of<cpp_forward_declarable, T>::value,
                                    type_safe::optional_ref<const T>>::type
        {
            if (!entity.definition())
                // entity is definition itself
                return type_safe::opt_cref(&entity);
            else
                // entity is not a definition
                // lookup the definition
                return idx
                    .lookup_definition(entity.definition().value())
                    // downcast
                    .map([](const cpp_entity& e) -> const T& {
                        DEBUG_ASSERT(e.kind() == T::kind(), detail::assert_handler{});
                        return static_cast<const T&>(e);
                    });
        }

        template <typename T>
        auto get_definition_impl(const cpp_entity_index&, const T& entity) ->
            typename std::enable_if<!std::is_base_of<cpp_forward_declarable, T>::value,
                                    type_safe::optional_ref<const T>>::type
        {
            return type_safe::opt_cref(&entity);
        }
    } // namespace detail

    /// Gets the definition of an entity.
    /// \returns A [ts::optional_ref]() to the entity that is the definition.
    /// If the entity is a definition or not derived from [cppast::cpp_forward_declarable](),
    /// returns a reference to the entity itself.
    /// Otherwise lookups the definition id and returns it.
    /// \requires `entity` must be derived from [cppast::cpp_entity]().
    /// \param 1
    /// \exclude
    template <typename T,
              typename = typename std::enable_if<std::is_base_of<cpp_entity, T>::value>::type>
    type_safe::optional_ref<const T> get_definition(const cpp_entity_index& idx, const T& entity)
    {
        return detail::get_definition_impl(idx, entity);
    }
} // namespace cppast

#endif // CPPAST_CPP_FORWARD_DECLARABLE_HPP_INCLUDED
