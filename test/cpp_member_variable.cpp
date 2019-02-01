// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_member_variable.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_member_variable")
{
    auto code = R"(
struct foo
{
    /// int a;
    int a;
    /// float b=3.14f;
    float b = 3.14f;
    /// mutable char c;
    mutable char c;
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_member_variable.cpp", code);
    auto count = test_visit<cpp_member_variable>(*file, [&](const cpp_member_variable& var) {
        if (var.name() == "a")
        {
            auto type = cpp_builtin_type::build(cpp_int);
            REQUIRE(equal_types(idx, var.type(), *type));
            REQUIRE(!var.default_value());
            REQUIRE(!var.is_mutable());
        }
        else if (var.name() == "b")
        {
            auto type = cpp_builtin_type::build(cpp_float);
            REQUIRE(equal_types(idx, var.type(), *type));

            // all initializers are unexposed
            auto def = cpp_unexposed_expression::build(cpp_builtin_type::build(cpp_float),
                                                       cpp_token_string::tokenize("3.14f"));
            REQUIRE(var.default_value());
            REQUIRE(equal_expressions(var.default_value().value(), *def));

            REQUIRE(!var.is_mutable());
        }
        else if (var.name() == "c")
        {
            auto type = cpp_builtin_type::build(cpp_char);
            REQUIRE(equal_types(idx, var.type(), *type));
            REQUIRE(!var.default_value());
            REQUIRE(var.is_mutable());
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}

TEST_CASE("cpp_bitfield")
{
    auto code = R"(
struct foo
{
    /// char a:3;
    char a : 3;
    /// mutable char b:2;
    mutable char b : 2;
    /// char:0;
    char : 0;
    /// char c:3;
    char c : 3;
    /// char:4;
    char : 4;
};
)";

    auto file  = parse({}, "cpp_bitfield.cpp", code);
    auto count = test_visit<cpp_bitfield>(*file, [&](const cpp_bitfield& bf) {
        REQUIRE(!bf.default_value());
        REQUIRE(equal_types({}, bf.type(), *cpp_builtin_type::build(cpp_char)));

        if (bf.name() == "a")
        {
            REQUIRE(bf.no_bits() == 3u);
            REQUIRE(!bf.is_mutable());
        }
        else if (bf.name() == "b")
        {
            REQUIRE(bf.no_bits() == 2u);
            REQUIRE(bf.is_mutable());
        }
        else if (bf.name() == "c")
        {
            REQUIRE(bf.no_bits() == 3u);
            REQUIRE(!bf.is_mutable());
        }
        else if (bf.name() == "")
        {
            REQUIRE(!bf.is_mutable());
            if (bf.no_bits() != 0u && bf.no_bits() != 4u)
            {
                INFO(bf.no_bits());
                REQUIRE(false);
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);
}
