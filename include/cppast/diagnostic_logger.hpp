// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_DIAGNOSTIC_LOGGER_HPP_INCLUDED
#define CPPAST_DIAGNOSTIC_LOGGER_HPP_INCLUDED

#include <type_safe/reference.hpp>

#include <cppast/diagnostic.hpp>

namespace cppast
{
/// Base class for a [cppast::diagnostic]() logger.
///
/// Its task is controlling how diagnostic are being displayed.
class diagnostic_logger
{
public:
    /// \effects Creates it either as verbose or not.
    explicit diagnostic_logger(bool is_verbose = false) noexcept : verbose_(is_verbose) {}

    diagnostic_logger(const diagnostic_logger&) = delete;
    diagnostic_logger& operator=(const diagnostic_logger&) = delete;
    virtual ~diagnostic_logger() noexcept                  = default;

    /// \effects Logs the diagnostic by invoking the `do_log()` member function.
    /// \returns Whether or not the diagnostic was logged.
    /// \notes `source` points to a string literal that gives additional context to what generates
    /// the message.
    bool log(const char* source, const diagnostic& d) const;

    /// \effects Sets whether or not the logger prints debugging diagnostics.
    void set_verbose(bool value) noexcept
    {
        verbose_ = value;
    }

    /// \returns Whether or not the logger prints debugging diagnostics.
    bool is_verbose() const noexcept
    {
        return verbose_;
    }

private:
    virtual bool do_log(const char* source, const diagnostic& d) const = 0;

    bool verbose_;
};

/// \returns The default logger object.
type_safe::object_ref<const diagnostic_logger> default_logger() noexcept;

/// \returns The default verbose logger object.
type_safe::object_ref<const diagnostic_logger> default_verbose_logger() noexcept;

/// A [cppast::diagnostic_logger]() that logs to `stderr`.
///
/// It prints all diagnostics in an implementation-defined format.
class stderr_diagnostic_logger final : public diagnostic_logger
{
public:
    using diagnostic_logger::diagnostic_logger;

private:
    bool do_log(const char* source, const diagnostic& d) const override;
};
} // namespace cppast

#endif // CPPAST_DIAGNOSTIC_LOGGER_HPP_INCLUDED
