// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_token.hpp>

#include <catch.hpp>

#include <algorithm>
#include <initializer_list>

using namespace cppast;

void check_equal_tokens(const std::string& str, std::initializer_list<cpp_token> tokens)
{
    auto token_str = cpp_token_string::tokenize(str);
    INFO(str);
    REQUIRE(token_str.end() - token_str.begin() == tokens.size());
    REQUIRE(std::equal(token_str.begin(), token_str.end(), tokens.begin()));
}

TEST_CASE("tokenizer")
{
    SECTION("integer literals")
    {
        check_equal_tokens("    1234   ", {cpp_token(cpp_token_kind::int_literal, "1234")});
        check_equal_tokens("1, 2", {cpp_token(cpp_token_kind::int_literal, "1"),
                                    cpp_token(cpp_token_kind::punctuation, ","),
                                    cpp_token(cpp_token_kind::int_literal, "2")});

        // integer suffixes
        check_equal_tokens("1234ul", {cpp_token(cpp_token_kind::int_literal, "1234ul")});
        check_equal_tokens("12'34LU", {cpp_token(cpp_token_kind::int_literal, "1234LU")});

        // other integer formats
        check_equal_tokens("01234", {cpp_token(cpp_token_kind::int_literal, "01234")});
        check_equal_tokens("0x1234AF", {cpp_token(cpp_token_kind::int_literal, "0x1234AF")});
        check_equal_tokens("0b101101", {cpp_token(cpp_token_kind::int_literal, "0b101101")});
    }
    SECTION("floating point literals")
    {
        // floating point suffixes
        check_equal_tokens("3.14", {cpp_token(cpp_token_kind::float_literal, "3.14")});
        check_equal_tokens("3.14f", {cpp_token(cpp_token_kind::float_literal, "3.14f")});
        check_equal_tokens("3.14L", {cpp_token(cpp_token_kind::float_literal, "3.14L")});

        // missing parts
        check_equal_tokens(".5", {cpp_token(cpp_token_kind::float_literal, ".5")});
        check_equal_tokens("1.", {cpp_token(cpp_token_kind::float_literal, "1.")});

        // exponents
        check_equal_tokens("1.0e4", {cpp_token(cpp_token_kind::float_literal, "1.0e4")});
        check_equal_tokens("1e4", {cpp_token(cpp_token_kind::float_literal, "1e4")});
        check_equal_tokens(".5e-2", {cpp_token(cpp_token_kind::float_literal, ".5e-2")});

        // hexadecimal
        check_equal_tokens("0xabc.def", {cpp_token(cpp_token_kind::float_literal, "0xabc.def")});
        check_equal_tokens("0x123p42", {cpp_token(cpp_token_kind::float_literal, "0x123p42")});
    }
    SECTION("character literals")
    {
        check_equal_tokens(R"('a')", {cpp_token(cpp_token_kind::char_literal, R"('a')")});
        check_equal_tokens(R"(u8'a')", {cpp_token(cpp_token_kind::char_literal, R"(u8'a')")});
        check_equal_tokens(R"(U'a')", {cpp_token(cpp_token_kind::char_literal, R"(U'a')")});
        check_equal_tokens(R"('\'')", {cpp_token(cpp_token_kind::char_literal, R"('\'')")});
    }
    SECTION("string literals")
    {
        check_equal_tokens(R"("hello")", {cpp_token(cpp_token_kind::string_literal, R"("hello")")});
        check_equal_tokens(R"(u8"he\"llo")",
                           {cpp_token(cpp_token_kind::string_literal, R"(u8"he\"llo")")});

        check_equal_tokens(R"*(R"(hel\"lo)")*",
                           {cpp_token(cpp_token_kind::string_literal, R"*(R"(hel\"lo)")*")});
        check_equal_tokens(R"**(R"*(hello R"(foo)")*")**",
                           {cpp_token(cpp_token_kind::string_literal,
                                      R"**(R"*(hello R"(foo)")*")**")});
    }
    SECTION("UDLs")
    {
        check_equal_tokens("123_foo", {cpp_token(cpp_token_kind::int_literal, "123_foo")});
        check_equal_tokens("123.456_foo",
                           {cpp_token(cpp_token_kind::float_literal, "123.456_foo")});
        check_equal_tokens(R"("hi"_foo)",
                           {cpp_token(cpp_token_kind::string_literal, R"("hi"_foo)")});
    }
    SECTION("identifiers")
    {
        check_equal_tokens("foo bar baz_a", {cpp_token(cpp_token_kind::identifier, "foo"),
                                             cpp_token(cpp_token_kind::identifier, "bar"),
                                             cpp_token(cpp_token_kind::identifier, "baz_a")});
        check_equal_tokens("constant", {cpp_token(cpp_token_kind::identifier, "constant")});
    }
    SECTION("keywords")
    {
        // just test some
        check_equal_tokens("const float auto", {cpp_token(cpp_token_kind::keyword, "const"),
                                                cpp_token(cpp_token_kind::keyword, "float"),
                                                cpp_token(cpp_token_kind::keyword, "auto")});
    }
    SECTION("punctuations")
    {
        // just test munch things
        check_equal_tokens("<< <= <", {cpp_token(cpp_token_kind::punctuation, "<<"),
                                       cpp_token(cpp_token_kind::punctuation, "<="),
                                       cpp_token(cpp_token_kind::punctuation, "<")});
        check_equal_tokens("- -- -> ->*", {cpp_token(cpp_token_kind::punctuation, "-"),
                                           cpp_token(cpp_token_kind::punctuation, "--"),
                                           cpp_token(cpp_token_kind::punctuation, "->"),
                                           cpp_token(cpp_token_kind::punctuation, "->*")});
        check_equal_tokens("--->>>>", {cpp_token(cpp_token_kind::punctuation, "--"),
                                       cpp_token(cpp_token_kind::punctuation, "->"),
                                       cpp_token(cpp_token_kind::punctuation, ">>"),
                                       cpp_token(cpp_token_kind::punctuation, ">")});

        // alternative spellings
        check_equal_tokens("and not xor", {cpp_token(cpp_token_kind::punctuation, "&&"),
                                           cpp_token(cpp_token_kind::punctuation, "!"),
                                           cpp_token(cpp_token_kind::punctuation, "^")});

        // digraphs
        check_equal_tokens("<% foo<::bar>", {cpp_token(cpp_token_kind::punctuation, "{"),
                                             cpp_token(cpp_token_kind::identifier, "foo"),
                                             cpp_token(cpp_token_kind::punctuation, "<"),
                                             cpp_token(cpp_token_kind::punctuation, "::"),
                                             cpp_token(cpp_token_kind::identifier, "bar"),
                                             cpp_token(cpp_token_kind::punctuation, ">")});
    }
}
