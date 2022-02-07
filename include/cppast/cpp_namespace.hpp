// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_NAMESPACE_HPP_INCLUDED
#define CPPAST_CPP_NAMESPACE_HPP_INCLUDED

#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_entity_ref.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a namespace.
class cpp_namespace final : public cpp_entity,
                            public cpp_entity_container<cpp_namespace, cpp_entity>
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builds a [cppast::cpp_namespace]().
    class builder
    {
    public:
        /// \effects Sets the namespace name and whether it is inline and nested.
        explicit builder(std::string name, bool is_inline, bool is_nested)
        : namespace_(new cpp_namespace(std::move(name), is_inline, is_nested))
        {}

        /// \effects Adds an entity.
        void add_child(std::unique_ptr<cpp_entity> child) noexcept
        {
            namespace_->add_child(std::move(child));
        }

        /// \returns The not yet finished namespace.
        cpp_namespace& get() const noexcept
        {
            return *namespace_;
        }

        /// \effects Registers the namespace in the [cppast::cpp_entity_index](),
        /// using the given [cppast::cpp_entity_id]().
        /// \returns The finished namespace.
        std::unique_ptr<cpp_namespace> finish(const cpp_entity_index& idx, cpp_entity_id id)
        {
            idx.register_namespace(std::move(id), type_safe::ref(*namespace_));
            return std::move(namespace_);
        }

    private:
        std::unique_ptr<cpp_namespace> namespace_;
    };

    /// \returns Whether or not the namespace is an `inline namespace`.
    bool is_inline() const noexcept
    {
        return inline_;
    }

    /// \returns Whether or not the namespace is part of a C++17 nested namespace.
    bool is_nested() const noexcept
    {
        return nested_;
    }

    /// \returns Whether or not the namespace is anonymous.
    bool is_anonymous() const noexcept
    {
        return name().empty();
    }

private:
    cpp_namespace(std::string name, bool is_inline, bool is_nested)
    : cpp_entity(std::move(name)), inline_(is_inline), nested_(is_nested)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    type_safe::optional<cpp_scope_name> do_get_scope_name() const override
    {
        return type_safe::ref(*this);
    }

    bool inline_;
    bool nested_;
};

/// \exclude
namespace detail
{
    struct cpp_namespace_ref_predicate
    {
        bool operator()(const cpp_entity& e);
    };
} // namespace detail

/// A reference to a [cppast::cpp_namespace]().
using cpp_namespace_ref = basic_cpp_entity_ref<cpp_namespace, detail::cpp_namespace_ref_predicate>;

/// A [cppast::cpp_entity]() modelling a namespace alias.
class cpp_namespace_alias final : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered namespace alias.
    static std::unique_ptr<cpp_namespace_alias> build(const cpp_entity_index& idx, cpp_entity_id id,
                                                      std::string name, cpp_namespace_ref target);

    /// \returns The [cppast::cpp_namespace_ref]() to the aliased namespace.
    /// \notes If the namespace aliases aliases another namespace alias,
    /// the target entity will still be the namespace, not the alias.
    const cpp_namespace_ref& target() const noexcept
    {
        return target_;
    }

private:
    cpp_namespace_alias(std::string name, cpp_namespace_ref target)
    : cpp_entity(std::move(name)), target_(std::move(target))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_namespace_ref target_;
};

/// A [cppast::cpp_entity]() modelling a using directive.
///
/// A using directive is `using namespace std`, for example.
/// \notes It does not have a name.
class cpp_using_directive final : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created using directive.
    /// \notes It is not meant to be registered at the [cppast::cpp_entity_index](),
    /// as nothing can refer to it.
    static std::unique_ptr<cpp_using_directive> build(cpp_namespace_ref target)
    {
        return std::unique_ptr<cpp_using_directive>(new cpp_using_directive(std::move(target)));
    }

    /// \returns The [cppast::cpp_namespace_ref]() that is being used.
    const cpp_namespace_ref& target() const
    {
        return target_;
    }

private:
    cpp_using_directive(cpp_namespace_ref target) : cpp_entity(""), target_(std::move(target)) {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_namespace_ref target_;
};

/// A [cppast::cpp_entity]() modelling a using declaration.
///
/// A using declaration is `using std::vector`, for example.
/// \notes It does not have a name.
class cpp_using_declaration final : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created using declaration.
    /// \notes It is not meant to be registered at the [cppast::cpp_entity_index](),
    /// as nothing can refer to it.
    static std::unique_ptr<cpp_using_declaration> build(cpp_entity_ref target)
    {
        return std::unique_ptr<cpp_using_declaration>(new cpp_using_declaration(std::move(target)));
    }

    /// \returns The [cppast::cpp_entity_ref]() that is being used.
    /// \notes The name of the reference is the same as the name of this entity.
    const cpp_entity_ref& target() const noexcept
    {
        return target_;
    }

private:
    cpp_using_declaration(cpp_entity_ref target) : cpp_entity(""), target_(std::move(target)) {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_entity_ref target_;
};
} // namespace cppast

#endif // CPPAST_CPP_NAMESPACE_HPP_INCLUDED
