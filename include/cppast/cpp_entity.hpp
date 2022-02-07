// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_ENTITY_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_HPP_INCLUDED

#include <atomic>
#include <string>

#include <type_safe/optional_ref.hpp>

#include <cppast/cpp_attribute.hpp>
#include <cppast/cpp_token.hpp>
#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{

/// The name of a scope.
///
/// It is a combination of a name and optional template parameters.
class cpp_scope_name
{
public:
    /// \effects Creates a scope out of a given entity.
    cpp_scope_name(type_safe::object_ref<const cpp_entity> entity);

    /// \returns The name of the scope.
    const std::string& name() const noexcept;

    /// \returns Whether or not the scope is templated.
    bool is_templated() const noexcept
    {
        return templ_.has_value();
    }

    /// \returns An iteratable object iterating over the [cppast::cpp_template_parameter]() entities
    /// of the scope. \requires The scope is templated.
    detail::iteratable_intrusive_list<cpp_template_parameter> template_parameters() const noexcept;

private:
    type_safe::object_ref<const cpp_entity>     entity_;
    type_safe::optional_ref<const cpp_template> templ_;
};

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
    type_safe::optional<cpp_scope_name> scope_name() const
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
    /// * A C style doc comment. It is a C style comment starting with an additional `*`, i.e.
    /// `/**`. One space after the leading sequence will be skipped. It ends either with `*/` or
    /// `**/`. After a newline all whitespace is skipped, as well as an optional `*` followed by
    /// another optional space, as well as trailing whitespace on each line. I.e. `/** a\n      * b
    /// */` yields the text `a\nb`.
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
        comment_ = comment.value_or("");
    }

    /// \returns The list of attributes that are specified for that entity.
    const cpp_attribute_list& attributes() const noexcept
    {
        return attributes_;
    }

    /// \effects Adds an attribute for that entity.
    void add_attribute(cpp_attribute attr) noexcept
    {
        attributes_.push_back(std::move(attr));
    }

    /// \effects Adds multiple arguments for that entity.
    void add_attribute(const cpp_attribute_list& list) noexcept
    {
        attributes_.insert(attributes_.end(), list.begin(), list.end());
    }

    /// \returns The specified user data.
    void* user_data() const noexcept
    {
        return user_data_.load();
    }

    /// \effects Sets some kind of user data.
    ///
    /// User data is just some kind of pointer, there are no requirements.
    /// The class will do no lifetime management.
    ///
    /// User data is useful if you need to store additional data for an entity without the need to
    /// maintain a registry.
    void set_user_data(void* data) const noexcept
    {
        user_data_ = data;
    }

    /// \effects Creates it giving it the the name.
    cpp_entity(std::string name) : name_(std::move(name)), user_data_(nullptr) {}

private:
    /// \returns The kind of the entity.
    virtual cpp_entity_kind do_get_entity_kind() const noexcept = 0;

    /// \returns The name of the new scope created by the entity, if any.
    /// By default, there is no scope created.
    virtual type_safe::optional<cpp_scope_name> do_get_scope_name() const
    {
        return type_safe::nullopt;
    }

    void on_insert(const cpp_entity& parent) noexcept
    {
        parent_ = type_safe::ref(parent);
    }

    std::string                               name_;
    std::string                               comment_;
    cpp_attribute_list                        attributes_;
    type_safe::optional_ref<const cpp_entity> parent_;
    mutable std::atomic<void*>                user_data_;

    template <typename T>
    friend struct detail::intrusive_list_access;
    friend detail::intrusive_list_node<cpp_entity>;
};

/// A [cppast::cpp_entity]() that isn't exposed directly.
///
/// The only information available is the raw source code.
class cpp_unexposed_entity final : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly built and registered unexposed entity.
    /// \notes It will be registered as a declaration.
    static std::unique_ptr<cpp_entity> build(const cpp_entity_index& index, cpp_entity_id id,
                                             std::string name, cpp_token_string spelling);

    /// \returns A newly built unnamed unexposed entity.
    /// It will not be registered.
    static std::unique_ptr<cpp_entity> build(cpp_token_string spelling);

    /// \returns The spelling of that entity.
    const cpp_token_string& spelling() const noexcept
    {
        return spelling_;
    }

private:
    cpp_unexposed_entity(std::string name, cpp_token_string spelling)
    : cpp_entity(std::move(name)), spelling_(std::move(spelling))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_token_string spelling_;
};

/// \returns Whether or not the entity is templated.
/// If this function returns `true` that means the entity is not the "real" entity,
/// but contains just the information for the template which is the parent entity.
/// \notes Do not use this entity other to read information from the template entity.
bool is_templated(const cpp_entity& e) noexcept;

/// \returns Whether or not the given entity is "friended",
/// that is, its declaration exists as part of a [cppast::cpp_friend]() declaration.
bool is_friended(const cpp_entity& e) noexcept;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_HPP_INCLUDED
