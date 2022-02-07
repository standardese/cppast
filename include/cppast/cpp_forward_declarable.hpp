// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

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

    /// \returns A reference to the semantic parent of the entity.
    /// This applies only to out-of-line definitions
    /// and is the entity which owns the declaration.
    const type_safe::optional<cpp_entity_ref>& semantic_parent() const noexcept
    {
        return semantic_parent_;
    }

    /// \returns The name of the semantic parent, if it has one,
    /// else the empty string.
    /// \notes This may include template parameters.
    std::string semantic_scope() const noexcept
    {
        return type_safe::copy(semantic_parent_.map(&cpp_entity_ref::name)).value_or("");
    }

protected:
    /// \effects Marks the entity as definition.
    /// \notes If it is not a definition,
    /// [*set_definition]() must be called.
    cpp_forward_declarable() noexcept = default;

    ~cpp_forward_declarable() noexcept = default;

    /// \effects Sets the definition entity,
    /// marking it as a forward declaration.
    void mark_declaration(cpp_entity_id def) noexcept
    {
        definition_ = std::move(def);
    }

    /// \effects Sets the semantic parent of the entity.
    void set_semantic_parent(type_safe::optional<cpp_entity_ref> semantic_parent) noexcept
    {
        semantic_parent_ = std::move(semantic_parent);
    }

private:
    type_safe::optional<cpp_entity_ref> semantic_parent_;
    type_safe::optional<cpp_entity_id>  definition_;
};

/// \returns Whether or not the given entity is a definition.
bool is_definition(const cpp_entity& e) noexcept;

/// Gets the definition of an entity.
/// \returns A [ts::optional_ref]() to the entity that is the definition.
/// If the entity is a definition or not derived from [cppast::cpp_forward_declarable]() (only valid
/// for the generic entity overload), returns a reference to the entity itself. Otherwise lookups
/// the definition id and returns it. \notes The return value will only be `nullptr`, if the
/// definition is not registered. \group get_definition
type_safe::optional_ref<const cpp_entity> get_definition(const cpp_entity_index& idx,
                                                         const cpp_entity&       e);
/// \group get_definition
type_safe::optional_ref<const cpp_enum> get_definition(const cpp_entity_index& idx,
                                                       const cpp_enum&         e);
/// \group get_definition
type_safe::optional_ref<const cpp_class> get_definition(const cpp_entity_index& idx,
                                                        const cpp_class&        e);
/// \group get_definition
type_safe::optional_ref<const cpp_variable> get_definition(const cpp_entity_index& idx,
                                                           const cpp_variable&     e);
/// \group get_definition
type_safe::optional_ref<const cpp_function_base> get_definition(const cpp_entity_index&  idx,
                                                                const cpp_function_base& e);

} // namespace cppast

#endif // CPPAST_CPP_FORWARD_DECLARABLE_HPP_INCLUDED
