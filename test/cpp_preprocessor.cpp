// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

    auto index = 0u;
    for (auto& child : *file)
    {
        if (child.kind() == cpp_entity_kind::include_directive_t)
            continue;
        REQUIRE(child.name() == order[index++]);
    }
}

// requires clang 4.0
TEST_CASE("cpp_include_directive", "[!hide][clang4]")
{
    write_file("cpp_include_directive-header.hpp", R"(
#define FOO
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

    auto count =
        test_visit<cpp_include_directive>(*file_a, [&](const cpp_include_directive& include) {
            if (include.name() == "iostream")
            {
                REQUIRE(include.target().name() == include.name());
                REQUIRE(include.include_kind() == cppast::cpp_include_kind::system);
                REQUIRE(include.target().get(idx).empty());
            }
            else if (include.name() == "cpp_include_directive-header.hpp")
            {
                REQUIRE(include.target().name() == include.name());
                REQUIRE(include.include_kind() == cppast::cpp_include_kind::local);
                REQUIRE(include.target().get(idx).empty());
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
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 1u);
}

TEST_CASE("comment matching")
{
    auto code = R"(
/// u

/// a
/// a
struct a {};

/// u
/** b
  * b */
void b(int, float);

/** u */
//! c
/// c
enum class c
{
    d, //< d
    /// d
    e, //< e
    /// e

    /** f
f **/
    f,
};

/// g
/// g
#define g(name) \
class name \
{ \
    /** i
        i */ \
    void i(); \
};

/// h
/// h
g(h)

/// j
/// j
using j = int;

/// k
/// k
template <typename T>
void k();
)";

    auto file = parse({}, "comment-matching.cpp", code);
    visit(*file, [&](const cpp_entity& e, visitor_info) {
        if (e.kind() == cpp_entity_kind::file_t)
            return true;
        else if (e.name().empty())
            return true;
        else if (is_templated(e))
            return true;

        INFO(e.name());
        REQUIRE(e.comment());
        REQUIRE(e.comment().value() == e.name() + "\n" + e.name());
        return true;
    });

    for (auto& comment : file->unmatched_comments())
        REQUIRE(comment == "u");
    REQUIRE((file->unmatched_comments().size() == 3u));
}
