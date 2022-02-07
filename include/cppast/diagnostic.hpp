// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_DIAGNOSTIC_HPP_INCLUDED
#define CPPAST_DIAGNOSTIC_HPP_INCLUDED

#include <sstream>
#include <string>

#include <type_safe/optional.hpp>

#include <cppast/cppast_fwd.hpp>

namespace cppast
{
/// Describes a physical source location attached to a [cppast::diagnostic]().
/// \notes All information might be unavailable.
struct source_location
{
    type_safe::optional<std::string> entity;
    type_safe::optional<std::string> file;
    type_safe::optional<unsigned>    line, column;

    /// \returns A source location where all information is available.
    static source_location make(std::string entity, std::string file, unsigned line,
                                unsigned column)
    {
        return {std::move(entity), std::move(file), line, column};
    }

    /// \returns A source location where only file information is available.
    static source_location make_file(std::string                   file,
                                     type_safe::optional<unsigned> line   = type_safe::nullopt,
                                     type_safe::optional<unsigned> column = type_safe::nullopt)
    {
        return {type_safe::nullopt, std::move(file), line, column};
    }

    /// \returns A source location where only the entity name is available.
    static source_location make_entity(std::string entity)
    {
        return {std::move(entity), type_safe::nullopt, type_safe::nullopt, type_safe::nullopt};
    }

    /// \returns A source location where no information is avilable.
    static source_location make_unknown()
    {
        return {type_safe::nullopt, type_safe::nullopt, type_safe::nullopt, type_safe::nullopt};
    }

    /// \returns A source location where entity and file name is available.
    static source_location make_entity(std::string entity, std::string file)
    {
        return {std::move(entity), std::move(file), type_safe::nullopt, type_safe::nullopt};
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
            {
                result += std::to_string(line.value());

                if (column)
                    result += "," + std::to_string(column.value());

                result += ":";
            }

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
    info,     //< An informational message.
    warning,  //< A warning that doesn't impact AST generation.
    error,    //< A non-critical error that does impact AST generation but not critically.
    critical, //< A critical error where AST generation isn't possible.
    /// \notes This will usually result in an exception being thrown after the diagnostic has been
    /// displayed.
};

/// \returns A human-readable string describing the severity.
inline const char* to_string(severity s) noexcept
{
    switch (s)
    {
    case severity::debug:
        return "debug";
    case severity::info:
        return "info";
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

namespace detail
{
    template <typename... Args>
    std::string format(Args&&... args)
    {
        std::ostringstream stream;
        int                dummy[] = {(stream << std::forward<Args>(args), 0)...};
        (void)dummy;
        return stream.str();
    }
} // namespace detail

/// Creates a diagnostic.
/// \returns A diagnostic with the specified severity and location.
/// The message is created by streaming each argument in order to a [std::ostringstream]().
template <typename... Args>
diagnostic format_diagnostic(severity sev, source_location loc, Args&&... args)
{
    return {detail::format(std::forward<Args>(args)...), std::move(loc), sev};
}
} // namespace cppast

#endif // CPPAST_DIAGNOSTIC_HPP_INCLUDED
