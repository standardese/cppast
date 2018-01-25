// Copyright (C) 2017-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
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

TEST_CASE("preprocessor line numbers")
{
    auto code = R"(/// 1

#include <iostream>

/// 5

#include <string>

int foo;

/// 11

#define foo \
    \
    \
    int \
    main()

/// 19

foo {}

/// 23

/* C comment
spanning
multiple
lines
*/

/**

*/

#include <vector>

/// 37
)";

    auto file = parse({}, "preprocessor_line_numbers.cpp", code);
    for (auto& comment : file->unmatched_comments())
    {
        if (comment.content[0] != '\n')
            REQUIRE(comment.line == std::stoi(comment.content));
    }
    REQUIRE((file->unmatched_comments().size() == 6u + 1u));
}

TEST_CASE("comment content")
{
    auto code = R"(
/// simple comment

///no space

/// multi
/// line
/// comment

/** C comment */
/**C comment no space*/

/** Multiline
C
  comment
with indent */

    /** Multiline
        C
         comment
          with
           indent */

    /** Multiline
            * C
            * comment
            * with
            * indent
            * star */
)";

    auto file     = parse({}, "comment-content.cpp", code);
    auto comments = file->unmatched_comments();
    REQUIRE((comments.size() == 8u));

    REQUIRE(comments[0u].content == "simple comment");
    REQUIRE(comments[1u].content == "no space");
    REQUIRE(comments[2u].content == "multi\nline\ncomment");
    REQUIRE(comments[3u].content == "C comment");
    REQUIRE(comments[4u].content == "C comment no space");
    REQUIRE(comments[5u].content == "Multiline\nC\n  comment\nwith indent");
    REQUIRE(comments[6u].content == "Multiline\nC\n comment\n  with\n   indent");
    REQUIRE(comments[7u].content == "Multiline\nC\ncomment\nwith\nindent\nstar");
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
void b(int, float)
{
    auto c = '#';
}

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
};

/// h
/// h
g(h)

/*! i
 i */
using i = int;

/// cstddef
/// cstddef
#include <cstddef>

/// j
/// j
template <typename T/**/>
void j();
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

    auto add = 0u;
    for (auto& comment : file->unmatched_comments())
    {
        if (comment.content == "cstddef\ncstddef")
            // happens if include parsing is not supported
            // error is still going to be detected because if it is supported, the entity will be matched above
            add = 1u;
        else
            REQUIRE(comment.content == "u");
    }
    REQUIRE((file->unmatched_comments().size() == 3u + add));
}
