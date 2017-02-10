// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_LIBCLANG_PARSER_HPP_INCLUDED
#define CPPAST_LIBCLANG_PARSER_HPP_INCLUDED

#include <cppast/parser.hpp>

namespace cppast
{
    /// Compilation config for the [cppast::libclang_parser]().
    class libclang_compile_config final : public compile_config
    {
    public:
        libclang_compile_config();

    private:
        void do_set_flags(cpp_standard standard, type_safe::flag_set<compile_flag> flags) override;

        void do_add_include_dir(std::string path) override;

        void do_add_macro_definition(std::string name, std::string definition) override;

        void do_remove_macro_definition(std::string name) override;
    };

    /// A parser that uses libclang.
    class libclang_parser final : public parser
    {
    public:
        libclang_parser();
        ~libclang_parser() noexcept override;

    private:
        std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, const std::string& path,
                                           const compile_config& config) const override;

        struct impl;
        std::unique_ptr<impl> pimpl_;
    };
} // namespace cppast

#endif // CPPAST_LIBCLANG_PARSER_HPP_INCLUDED
