// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/parser.hpp>

#include <catch2/catch.hpp>

using namespace cppast;

TEST_CASE("parse_files")
{
    class null_compile_config : public compile_config
    {
    public:
        null_compile_config() : compile_config({}) {}

    private:
        void do_set_flags(cpp_standard, compile_flags) override {}

        void do_add_include_dir(std::string) override {}

        void do_add_macro_definition(std::string, std::string) override {}

        void do_remove_macro_definition(std::string) override {}

        const char* do_get_name() const noexcept override
        {
            return "null";
        }
    } config;

    class null_parser : public parser
    {
    public:
        using config = null_compile_config;

        null_parser() : parser(type_safe::ref(logger_)) {}

    private:
        std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                           const compile_config&) const override
        {
            return cpp_file::builder(std::move(path)).finish(idx);
        }

        stderr_diagnostic_logger logger_;
    };

    cpp_entity_index                idx;
    simple_file_parser<null_parser> parser(type_safe::ref(idx));

    auto file_names = {"a.cpp", "b.cpp", "c.cpp"};
    parse_files(parser, file_names, config);

    auto iter = file_names.begin();
    for (auto& file : parser.files())
        REQUIRE(file.name() == *iter++);
}
