// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ATTRIBUTE_HPP_INCLUDED
#define CPPAST_CPP_ATTRIBUTE_HPP_INCLUDED

#include <string>
#include <vector>

#include <type_safe/optional.hpp>

#include <cppast/cpp_token.hpp>

namespace cppast
{
    /// The known C++ attributes.
    enum class cpp_attribute_kind
    {
        // update get_attribute_kind() in tokenizer, when updating this

        alignas_,
        carries_dependency,
        deprecated,
        fallthrough,
        maybe_unused,
        nodiscard,
        noreturn,

        unknown, //< An unknown attribute.
    };

    namespace detail
    {
        inline const char* get_attribute_name(cpp_attribute_kind kind)
        {
            switch (kind)
            {
            case cpp_attribute_kind::alignas_:
                return "alignas";
            case cpp_attribute_kind::carries_dependency:
                return "carries_dependency";
            case cpp_attribute_kind::deprecated:
                return "deprecated";
            case cpp_attribute_kind::fallthrough:
                return "fallthrough";
            case cpp_attribute_kind::maybe_unused:
                return "maybe_unused";
            case cpp_attribute_kind::nodiscard:
                return "nodiscard";
            case cpp_attribute_kind::noreturn:
                return "noreturn";

            case cpp_attribute_kind::unknown:
                return "unknown";
            }

            return "<error>";
        }
    } // namespace detail

    /// A C++ attribute, including `alignas` specifiers.
    ///
    /// It consists of a name, an optional namespace scope and optional arguments.
    /// The scope is just a single identifier and doesn't include the `::` and can be given explicitly or via using.
    /// The arguments are as specified in the source code but do not include the outer-most `(` and `)`.
    /// It can also be variadic or not.
    ///
    /// An attribute can be known or unknown.
    /// A known attribute will have the [cppast::cpp_attribute_kind]() set properly.
    class cpp_attribute
    {
    public:
        /// \effects Creates a known attribute, potentially with arguments.
        cpp_attribute(cpp_attribute_kind kind, type_safe::optional<cpp_token_string> arguments)
        : cpp_attribute(type_safe::nullopt, detail::get_attribute_name(kind), std::move(arguments),
                        false)
        {
            kind_ = kind;
        }

        /// \effects Creates an unknown attribute giving it the optional scope, names, arguments and whether it is variadic.
        cpp_attribute(type_safe::optional<std::string> scope, std::string name,
                      type_safe::optional<cpp_token_string> arguments, bool is_variadic)
        : scope_(std::move(scope)),
          arguments_(std::move(arguments)),
          name_(std::move(name)),
          variadic_(is_variadic)
        {
        }

        /// \returns The kind of attribute, if it is known.
        const cpp_attribute_kind& kind() const noexcept
        {
            return kind_;
        }

        /// \returns The name of the attribute.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns The scope of the attribute, if there is any.
        const type_safe::optional<std::string>& scope() const noexcept
        {
            return scope_;
        }

        /// \returns Whether or not the attribute is variadic.
        bool is_variadic() const noexcept
        {
            return variadic_;
        }

        /// \returns The arguments of the attribute, if they are any.
        const type_safe::optional<cpp_token_string>& arguments() const noexcept
        {
            return arguments_;
        }

    private:
        type_safe::optional<std::string>      scope_;
        type_safe::optional<cpp_token_string> arguments_;
        std::string                           name_;
        cpp_attribute_kind                    kind_ = cpp_attribute_kind::unknown;
        bool                                  variadic_;
    };

    /// A list of C++ attributes.
    using cpp_attribute_list = std::vector<cpp_attribute>;
} // namespace cppast

#endif // CPPAST_CPP_ATTRIBUTE_HPP_INCLUDED
