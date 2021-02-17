#include <catch2/catch.hpp>
#include <fstream>

#include "libclang/preprocessor.hpp"
#include "test_parser.hpp"

#include <cppast/cpp_variable.hpp>

using namespace cppast;

TEST_CASE("preprocessor escaped character", "[!hide][clang4]")
{
    write_file("ppec.hpp", R"(
)");
    // This is an actual macro from the rapidjson source (reader.h)
    write_file("ppec.cpp", R"(
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
 static const char escape[256] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'/',
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
            0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
            0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        };
#undef Z16

#include "ppec.hpp"
)");

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);

    SECTION("fast")
    {
        config.fast_preprocessing(true);
    }
    SECTION("normal")
    {
        config.fast_preprocessing(false);
    }

    auto preprocessed = detail::preprocess(config, "ppec.cpp", default_logger().get());
    REQUIRE(preprocessed.includes.size() == 1);
    REQUIRE(preprocessed.includes[0].file_name == "ppec.hpp");
}

TEST_CASE("preprocessing use external macro")
{
    bool fast_preprocessing = false;
    SECTION("fast_preprocessing")
    {
        fast_preprocessing = true;
    }
    SECTION("normal")
    {
        fast_preprocessing = false;
    }

    auto file = parse({}, "preprocessing_external_macro.cpp", R"(
#include <climits>

/// auto result=8;
auto result = CHAR_BIT;
)",
                      fast_preprocessing);

    test_visit<cpp_variable>(*file, [&](const cpp_variable&) {});
}

TEST_CASE("fast_preprocessing include guard")
{
    auto file_name = "fast_preprocessing_include_guard.hpp";
    write_file(file_name, R"(
// This is a C++ comment that should get skipped.


   /// This as well.

 /* C style comments

#ifndef FALSE_POSITIVE
#define FALSE_POSITIVE

*/ #ifndef INCLUDE_GUARD // the include guard macro
        #define    INCLUDE_GUARD

struct foo {};

#endif
)");

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);
    config.fast_preprocessing(true);

    try
    {
        auto result = detail::preprocess(config, file_name, default_logger().get());
        REQUIRE(result.macros.size() == 1u);
        REQUIRE(result.macros[0].macro->name() == "INCLUDE_GUARD");
    }
    catch (libclang_error& ex)
    {
        FAIL(ex.what());
    }
}

TEST_CASE("preprocessor line numbers")
{
    bool fast_preprocessing = false;
    SECTION("fast_preprocessing")
    {
        fast_preprocessing = true;
    }
    SECTION("normal")
    {
        fast_preprocessing = false;
    }

    auto code = R"(/// 1

#include <iostream>

/// 5

#include <string>

int var;

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

    auto file = parse({}, "preprocessor_line_numbers.cpp", code, fast_preprocessing);
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
    bool fast_preprocessing = false;
    SECTION("fast_preprocessing")
    {
        fast_preprocessing = true;
    }
    SECTION("normal")
    {
        fast_preprocessing = false;
    }

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

    auto file = parse({}, "comment-matching.cpp", code, fast_preprocessing);
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
            // error is still going to be detected because if it is supported, the entity will be
            // matched above
            add = 1u;
        else
            REQUIRE(comment.content == "u");
    }
    REQUIRE((file->unmatched_comments().size() == 3u + add));
}
