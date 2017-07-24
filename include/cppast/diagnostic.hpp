// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DIAGNOSTIC_HPP_INCLUDED
#define CPPAST_DIAGNOSTIC_HPP_INCLUDED

#include <string>
#include <atomic>

#include <type_safe/optional.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>

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
        static source_location make_file(std::string file, unsigned line)
        {
            return {type_safe::nullopt, std::move(file), line};
        }

        /// \returns A source location where only the file is available.
        static source_location make_file(std::string file)
        {
            return {type_safe::nullopt, std::move(file), type_safe::nullopt};
        }

        /// \returns A source location where only the entity name is available.
        static source_location make_entity(std::string entity)
        {
            return {std::move(entity), type_safe::nullopt, type_safe::nullopt};
        }

        /// \returns A source location with no information available
        static source_location make_unknown()
        {
            return {type_safe::nullopt, type_safe::nullopt, type_safe::nullopt};
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
                    result += std::to_string(line.value()) + ":";
                if (entity)
                    result += " (" + entity.value() + "):";
            }
            else if (entity && !entity.value().empty())
                result += entity.value() + ":";

            return result;
        }
    };

    /// The severity of a [cppast::diagnostic]().
    enum class severity
    {
        debug,    //< A debug diagnostic that is just for debugging purposes.
        warning,  //< A warning that doesn't impact AST generation.
        error,    //< A non-critical error that does impact AST generation but not critically.
        critical, //< A critical error where AST generation isn't possible.
        /// \notes This will usually result in an exception being thrown after the diagnostic has been displayed.
    };

    /// \returns A human-readable string describing the severity.
    inline const char* to_string(severity s) noexcept
    {
        switch (s)
        {
        case severity::debug:
            return "debug";
        case severity::warning:
            return "warning";
        case severity::error:
            return "error";
        case severity::critical:
            return "critical";
        }

        return "programmer error";
    }

    /// A diagnostic.
    ///
    /// It represents an error message from a [cppast::parser]().
    struct diagnostic
    {
        std::string      message;
        source_location  location;
        cppast::severity severity;
    };

    /// Returns a [cppast::diagnostic]() with the given information
    ///
    /// This function composes a diagnostic from the given severity and location,
    /// with the message formatted from args
    template<typename... Args>
    diagnostic make_diagnostic(severity severity, const source_location& location, Args&&... args)
    {
        return {
            fmt::format(std::forward<Args>(args)...),
            location,
            severity
        };
    }
} // namespace cppast

#endif // CPPAST_DIAGNOSTIC_HPP_INCLUDED
