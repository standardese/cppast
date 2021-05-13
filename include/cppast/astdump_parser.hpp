// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_ASTDUMP_PARSER_HPP_INCLUDED
#define CPPAST_ASTDUMP_PARSER_HPP_INCLUDED

#include <stdexcept>

#include <cppast/parser.hpp>

namespace cppast
{
/// The exception thrown when a fatal parse error occurs.
class astdump_error final : public std::runtime_error
{
public:
    /// \effects Creates it with a message.
    astdump_error(std::string msg) : std::runtime_error(std::move(msg)) {}
};

/// Compilation config for the [cppast::astdump_parser]().
class astdump_compile_config final : public compile_config
{
public:
    /// Creates the default configuration.
    ///
    /// \effects It will set the clang binary determined by the build system,
    /// as well as the libclang system include directory determined by the build system.
    /// It will also define `__cppast__` with the value `"libclang"` as well as `__cppast_major__`
    /// and `__cppast_minor__`.
    astdump_compile_config();

    /// Creates the default configuration but using the specific clang binary.
    explicit astdump_compile_config(std::string clang_binary_path);

    astdump_compile_config(const astdump_compile_config& other) = default;
    astdump_compile_config& operator=(const astdump_compile_config& other) = default;

private:
    std::vector<std::string> get_ast_dump_cmd() const;

    void do_set_flags(cpp_standard standard, compile_flags flags) override;

    bool do_enable_feature(std::string name) override;

    void do_add_include_dir(std::string path) override;

    void do_add_macro_definition(std::string name, std::string definition) override;

    void do_remove_macro_definition(std::string name) override;

    const char* do_get_name() const noexcept override
    {
        return "astdump";
    }

    std::string clang_binary_;

    friend class astdump_parser;
};

/// A parser that uses clang's ast dump facility.
class astdump_parser final : public parser
{
public:
    using config = astdump_compile_config;

    /// \effects Creates a parser using the default logger.
    astdump_parser() : parser(default_logger()) {}

    /// \effects Creates a parser that will log error messages using the specified logger.
    explicit astdump_parser(type_safe::object_ref<const diagnostic_logger> logger) : parser(logger)
    {}

private:
    std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                       const compile_config& config) const override;
};
} // namespace cppast

#endif // CPPAST_ASTDUMP_PARSER_HPP_INCLUDED
