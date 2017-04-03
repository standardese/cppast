// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_LIBCLANG_PARSER_HPP_INCLUDED
#define CPPAST_LIBCLANG_PARSER_HPP_INCLUDED

#include <stdexcept>

#include <cppast/parser.hpp>

namespace cppast
{
    class libclang_compile_config;

    namespace detail
    {
        struct libclang_compile_config_access
        {
            static const std::string& clang_binary(const libclang_compile_config& config);

            static int clang_version(const libclang_compile_config& config);

            static const std::vector<std::string>& flags(const libclang_compile_config& config);
        };
    } // namespace detail

    /// Compilation config for the [cppast::libclang_parser]().
    class libclang_compile_config final : public compile_config
    {
    public:
        libclang_compile_config();

        /// \effects Sets the path to the location of the `clang++` binary and the version of that binary.
        /// \notes It will be used for preprocessing.
        void set_clang_binary(std::string binary, int major, int minor, int patch)
        {
            clang_binary_  = std::move(binary);
            clang_version_ = major * 10000 + minor * 100 + patch;
        }

    private:
        void do_set_flags(cpp_standard standard, type_safe::flag_set<compile_flag> flags) override;

        void do_add_include_dir(std::string path) override;

        void do_add_macro_definition(std::string name, std::string definition) override;

        void do_remove_macro_definition(std::string name) override;

        const char* do_get_name() const noexcept override
        {
            return "libclang";
        }

        std::string clang_binary_;
        int         clang_version_;

        friend detail::libclang_compile_config_access;
    };

    /// The exception thrown when a fatal parse error occurs.
    class libclang_error final : public std::runtime_error
    {
    public:
        /// \effects Creates it with a message.
        libclang_error(std::string msg) : std::runtime_error(std::move(msg))
        {
        }
    };

    /// A parser that uses libclang.
    class libclang_parser final : public parser
    {
    public:
        explicit libclang_parser(type_safe::object_ref<const diagnostic_logger> logger);
        ~libclang_parser() noexcept override;

    private:
        std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                           const compile_config& config) const override;

        struct impl;
        std::unique_ptr<impl> pimpl_;
    };
} // namespace cppast

#endif // CPPAST_LIBCLANG_PARSER_HPP_INCLUDED
