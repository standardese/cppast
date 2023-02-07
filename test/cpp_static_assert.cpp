// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_static_assert.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_static_assert")
{
    auto code = R"(
/// static_assert(true,"");
static_assert(true, "");
/// static_assert(true||false,"a");
static_assert(true || false, "a");

template <bool B>
struct foo
{
    /// static_assert(!B,"b");
    static_assert(!B, "b");
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_static_assert.cpp", code);
    auto count = test_visit<cpp_static_assert>(*file, [&](const cpp_static_assert& assert) {
        auto bool_t = cpp_builtin_type::build(cpp_builtin_type_kind::cpp_bool);

        REQUIRE(assert.name().empty());
        if (assert.message() == "")
            REQUIRE(equal_expressions(assert.expression(),
                                      *cpp_literal_expression::build(std::move(bool_t), "true")));
        else if (assert.message() == "a")
            REQUIRE(equal_expressions(assert.expression(),
                                      *cpp_unexposed_expression::build(std::move(bool_t),
                                                                       cpp_token_string::tokenize(
                                                                           "true||false"))));
        else if (assert.message() == "b")
            REQUIRE(equal_expressions(assert.expression(),
                                      *cpp_unexposed_expression::build(std::move(bool_t),
                                                                       cpp_token_string::tokenize(
                                                                           "!B"))));
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}
