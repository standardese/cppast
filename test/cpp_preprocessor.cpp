// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_preprocessor.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_macro_definition")
{
    auto        code    = R"(
#include <iostream>
#define G
/// #define A
#define A
/// #define B hello
#define B hello
namespace ns {}
/// #define C(x,y) x##_name
#define C(x, y) x##_name
/// #define D(...) __VA_ARGS__
#define D(...) __VA_ARGS__
/// #define E() barbaz
#define E() bar\
baz
namespace ns2
{
/// #define F () bar
#define F () bar
#undef G
}
)";
    const char* order[] = {"A", "B", "ns", "C", "D", "E", "ns2", "F"};

    auto check_macro
        = [](const cpp_macro_definition& macro, const char* replacement, const char* args) {
              REQUIRE(macro.replacement() == replacement);
              if (args)
              {
                  REQUIRE(macro.is_function_like());

                  std::string params;
                  for (auto& param : macro.parameters())
                  {
                      if (!params.empty())
                          params += ",";
                      params += param.name();
                  }
                  if (macro.is_variadic())
                  {
                      if (!params.empty())
                          params += ",";
                      params += "...";
                  }

                  REQUIRE(params == args);
              }
              else
              {
                  REQUIRE(!macro.is_function_like());
                  REQUIRE(!macro.is_variadic());
                  REQUIRE(macro.parameters().empty());
              }
          };

    auto file  = parse({}, "cpp_macro_definition.cpp", code);
    auto count = test_visit<cpp_macro_definition>(*file, [&](const cpp_macro_definition& macro) {
        if (macro.name() == "A")
            check_macro(macro, "", nullptr);
        else if (macro.name() == "B")
            check_macro(macro, "hello", nullptr);
        else if (macro.name() == "C")
            check_macro(macro, "x##_name", "x,y");
        else if (macro.name() == "D")
            check_macro(macro, "__VA_ARGS__", "...");
        else if (macro.name() == "E")
            check_macro(macro, "barbaz", "");
        else if (macro.name() == "F")
            check_macro(macro, "() bar", nullptr);
        else
            REQUIRE(false);
    });
    REQUIRE(count == 6u);

    auto index = 0u;
    for (auto& child : *file)
    {
        if (child.kind() == cpp_entity_kind::include_directive_t)
            continue;
        REQUIRE(child.name() == order[index++]);
    }
}

TEST_CASE("command line macro definition")
{
    write_file("command_line_macro_definition.hpp", "NAME foo;");
    write_file("command_line_macro_definition.cpp",
               R"(#include "command_line_macro_definition.hpp")");

    struct test_logger : diagnostic_logger
    {
        mutable bool error = false;

        bool do_log(const char* source, const diagnostic& d) const override
        {
            error = true;
            default_logger()->log(source, d);
            return false;
        }
    } logger;

    libclang_compile_config config;
    config.define_macro("NAME", "int");

    libclang_parser parser{type_safe::ref(logger)};
    parser.parse({}, "command_line_macro_definition.cpp", config);
    REQUIRE(!parser.error());
    REQUIRE(!logger.error);
}

TEST_CASE("cpp_include_directive")
{
    write_file("cpp_include_directive-header.hpp", R"(
#define FOO a\
b
)");

    auto header_a = R"(
/// #include <iostream>
#include <iostream>
/// #include "cpp_include_directive-header.hpp"
#include "cpp_include_directive-header.hpp"
)";

    auto header_b = R"(
/// #include "header_a.hpp"
#include "header_a.hpp"
)";

    cpp_entity_index idx;
    auto             file_a = parse(idx, "header_a.hpp", header_a);
    auto             file_b = parse(idx, "header_b.hpp", header_b);

    auto count
        = test_visit<cpp_include_directive>(*file_a, [&](const cpp_include_directive& include) {
              if (include.name() == "iostream")
              {
                  REQUIRE(include.target().name() == include.name());
                  REQUIRE(include.include_kind() == cppast::cpp_include_kind::system);
                  REQUIRE(include.target().get(idx).empty());
                  REQUIRE_THAT(include.full_path(), Catch::EndsWith("iostream"));
              }
              else if (include.name() == "cpp_include_directive-header.hpp")
              {
                  REQUIRE(include.target().name() == include.name());
                  REQUIRE(include.include_kind() == cppast::cpp_include_kind::local);
                  REQUIRE(include.target().get(idx).empty());
                  REQUIRE(include.full_path() == "./cpp_include_directive-header.hpp");
              }
              else
                  REQUIRE(false);
          });
    REQUIRE(count == 2u);

    count = test_visit<cpp_include_directive>(*file_b, [&](const cpp_include_directive& include) {
        if (include.name() == "header_a.hpp")
        {
            REQUIRE(include.target().name() == include.name());
            REQUIRE(include.include_kind() == cppast::cpp_include_kind::local);
            REQUIRE(
                equal_ref(idx, include.target(), cpp_file_ref(cpp_entity_id(""), "header_a.hpp")));
            REQUIRE(include.full_path() == "./header_a.hpp");
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 1u);
}
