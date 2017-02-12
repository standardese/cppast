// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_preprocessor.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_macro_definition")
{
    auto code = R"(
#include <iostream>
#define G
#define A
#define B hello
#define C(x, y) x##_name
#define D(...) __VA_ARGS__
#define E() bar\
baz
#define F () bar
#undef G
)";

    auto check_macro = [](const cpp_macro_definition& macro, const char* replacement,
                          const char* args) {
        REQUIRE(macro.replacement() == replacement);
        if (args)
        {
            REQUIRE(macro.is_function_like());
            REQUIRE(macro.parameters().value() == args);
        }
        else
        {
            REQUIRE(!macro.is_function_like());
            REQUIRE(!macro.parameters().has_value());
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
}
