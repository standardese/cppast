// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSER_HPP_INCLUDED
#define CPPAST_PARSER_HPP_INCLUDED

#include <atomic>

#include <cppast/compile_config.hpp>
#include <cppast/cpp_file.hpp>

namespace cppast
{
    class cpp_entity_index;
    struct diagnostic;

    /// Base class for a [cppast::diagnostic]() logger.
    ///
    /// Its task is controlling how diagnostic are being displayed.
    class diagnostic_logger
    {
    public:
        diagnostic_logger() noexcept : error_(false)
        {
        }

        diagnostic_logger(const diagnostic_logger&) = delete;
        diagnostic_logger& operator=(const diagnostic_logger&) = delete;
        virtual ~diagnostic_logger() noexcept                  = default;

        /// \effects Logs the diagnostic by invoking the `do_log()` member function.
        /// \returns Whether or not the diagnostic was logged.
        /// \notes `source` points to a string literal that gives additional context to what generates the message.
        bool log(const char* source, const diagnostic& d) const;

        /// \returns Whether or not a diagnostic of [severity::error]() was logged.
        /// \notes If an error happens, the parser will still continue to build the AST,
        /// unless the error is critical
        /// so the result will be incomplete if this function returns `true`.
        bool error_logged() const noexcept
        {
            return error_;
        }

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

        mutable std::atomic<bool> error_;
        bool                      verbose_ = false;
    };

    /// A [cppast::diagnostic_logger]() that logs to `stderr`.
    ///
    /// It prints all diagnostics in an implementation-defined format.
    class stderr_diagnostic_logger final : public diagnostic_logger
    {
    private:
        bool do_log(const char* source, const diagnostic& d) const override;
    };

    /// Base class for a parser.
    ///
    /// It reads a C++ source file and creates the matching [cppast::cpp_file]().
    class parser
    {
    public:
        parser(const parser&) = delete;
        parser& operator=(const parser&) = delete;

        virtual ~parser() noexcept = default;

        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        /// \requires The dynamic type of `config` must match the required config type.
        std::unique_ptr<cpp_file> parse(const cpp_entity_index& idx, std::string path,
                                        const compile_config& config) const
        {
            return do_parse(idx, std::move(path), config);
        }

    protected:
        /// \effects Creates it giving it a reference to the logger it uses.
        explicit parser(type_safe::object_ref<const diagnostic_logger> logger) : logger_(logger)
        {
        }

        /// \returns A reference to the logger used.
        const diagnostic_logger& logger() const noexcept
        {
            return *logger_;
        }

    private:
        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        virtual std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                                   const compile_config& config) const = 0;

        type_safe::object_ref<const diagnostic_logger> logger_;
    };
} // namespace cppast

#endif // CPPAST_PARSER_HPP_INCLUDED
