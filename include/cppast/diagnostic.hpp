// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DIAGNOSTIC_HPP_INCLUDED
#define CPPAST_DIAGNOSTIC_HPP_INCLUDED

#include <string>

#include <type_safe/optional.hpp>

namespace cppast
{
    /// Describes a physical source location attached to a [cppast::diagnostic]().
    /// \notes All information might be unavailable.
    struct source_location
    {
        type_safe::optional<std::string> entity;
        type_safe::optional<std::string> file;
        type_safe::optional<unsigned>    line;

        /// \returns A source location where all information is available.
        static source_location make(std::string entity, std::string file, unsigned line)
        {
            return {std::move(entity), std::move(file), line};
        }

        /// \returns A source location where only file and line information is available.
        static source_location make(std::string file, unsigned line)
        {
            return {type_safe::nullopt, std::move(file), line};
        }

        /// \returns A source location where only the entity name is available.
        static source_location make(std::string entity)
        {
            return {std::move(entity), type_safe::nullopt, type_safe::nullopt};
        }

        /// \returns A possible string representation of the source location.
        /// \notes It will include a separator, but no trailing whitespace.
        std::string to_string() const
        {
            std::string result;
            if (file)
            {
                result += file.value() + ":";
                if (line)
                    result += line.value() + ":";
                if (entity)
                    result += " (" + entity.value() + "):";
            }
            else if (entity)
                result += entity.value() + ":";

            return result;
        }
    };

    /// The severity of a [cppast::diagnostic]().
    enum class severity
    {
        warning,  //< A warning that doesn't impact AST generation.
        error,    //< A non-critical error that does impact AST generation but not critically.
        critical, //< A critical error where AST generation isn't possible.
        /// \notes This will usually result in an exception being thrown after the diagnostic has been displayed.
    };

    /// A diagnostic.
    ///
    /// It represents an error message from a [cppast::parser]().
    struct diagnostic
    {
        std::string      message;
        source_location  location;
        cppast::severity severity;
    };
} // namespace cppast

#endif // CPPAST_DIAGNOSTIC_HPP_INCLUDED
