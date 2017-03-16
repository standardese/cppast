// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_HPP_INCLUDED

#include <type_safe/optional_ref.hpp>

#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{
    enum class cpp_entity_kind;
    class cpp_entity_index;
    struct cpp_entity_id;

    /// The base class for all entities in the C++ AST.
    class cpp_entity : detail::intrusive_list_node<cpp_entity>
    {
    public:
        cpp_entity(const cpp_entity&) = delete;
        cpp_entity& operator=(const cpp_entity&) = delete;

        virtual ~cpp_entity() noexcept = default;

        /// \returns The kind of the entity.
        cpp_entity_kind kind() const noexcept
        {
            return do_get_entity_kind();
        }

        /// \returns The name of the entity.
        /// The name is the string associated with the entity's declaration.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns The name of the new scope created by the entity,
        /// if there is any.
        type_safe::optional<std::string> scope_name() const
        {
            return do_get_scope_name();
        }

        /// \returns A [ts::optional_ref]() to the parent entity in the AST.
        type_safe::optional_ref<const cpp_entity> parent() const noexcept
        {
            return parent_;
        }

        /// \returns The documentation comment associated with that entity, if any.
        /// \notes A documentation comment can have three forms:
        ///
        /// * A C style doc comment. It is a C style comment starting with an additional `*`, i.e. `/**`.
        /// One space after the leading sequence will be skipped.
        /// It ends either with `*/` or `**/`.
        /// After a newline all whitespace is skipped, as well as an optional `*` followed by another optional space,
        /// as well as trailing whitespace on each line.
        /// I.e. `/** a\n      * b */` yields the text `a\nb`.
        /// * A C++ style doc comment. It is a C++ style comment starting with an additional `/` or '!`,
        /// i.e. `///` or `//!`.
        /// One space character after the leading sequence will be skipped,
        /// as well as any trailing whitespace.
        /// Two C++ style doc comments on two adjacent lines will be merged.
        /// * An end of line doc comment. It is a C++ style comment starting with an '<', i.e. `//<`.
        /// One space character after the leading sequence will be skipped,
        /// as well as any trailing whitespace.
        /// If the next line is a C++ style doc comment, it will be merged with that one.
        ///
        /// A documentation comment is associated with an entity,
        /// if for C and C++ style doc comments, the entity declaration begins
        /// on the line after the last line of the comment,
        /// and if for an end of line comment, the entity declaration ends
        /// on the same line as the end of line comment.
        ///
        /// This comment system is also used by [standardese](https://standardese.foonathan.net).
        type_safe::optional_ref<const std::string> comment() const noexcept
        {
            return comment_.empty() ? nullptr : type_safe::opt_ref(&comment_);
        }

        /// \effects Sets the associated comment.
        /// \requires The comment must not be empty, if there is one.
        void set_comment(type_safe::optional<std::string> comment) noexcept
        {
            comment_ = std::move(comment.value());
        }

    protected:
        /// \effects Creates it giving it the the name.
        cpp_entity(std::string name) : name_(std::move(name))
        {
        }

    private:
        /// \returns The kind of the entity.
        virtual cpp_entity_kind do_get_entity_kind() const noexcept = 0;

        /// \returns The name of the new scope created by the entity, if any.
        /// By default, there is no scope created.
        virtual type_safe::optional<std::string> do_get_scope_name() const
        {
            return type_safe::nullopt;
        }

        void on_insert(const cpp_entity& parent) noexcept
        {
            parent_ = parent;
        }

        std::string                               name_;
        std::string                               comment_;
        type_safe::optional_ref<const cpp_entity> parent_;

        template <typename T>
        friend struct detail::intrusive_list_access;
        friend detail::intrusive_list_node<cpp_entity>;
    };

    /// \returns The full name of the [cppast::cpp_entity](), with all scopes.
    std::string full_name(const cpp_entity& e);
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_HPP_INCLUDED
