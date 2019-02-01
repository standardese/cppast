// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_enum.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_enum")
{
    auto code = R"(
/// enum a{
///   a_a,
///   a_b=42,
///   a_c,
///   a_d=a_a+2
/// };
enum a
{
    a_a,
    a_b = 42,
    a_c,
    a_d = a_a + 2,
};

/// enum class b;
enum class b; // forward declaration

/// enum class b
/// :int{
///   b_a,
///   b_b=42,
///   b_c
/// };
enum class b : int
{
    b_a,
    b_b = 42,
    b_c
};

namespace ns
{
    /// enum c
    /// :int;
    enum c : int;
}

/// enum ns::c
/// :int{
/// };
enum ns::c : int {};
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_enum.cpp", code);
    auto             count = test_visit<cpp_enum>(*file, [&](const cpp_enum& e) {
        if (e.name() == "a")
        {
            REQUIRE(e.is_definition());
            REQUIRE(!e.is_declaration());
            REQUIRE(!e.is_scoped());
            REQUIRE(!e.has_explicit_type());

            auto no_vals = 0u;
            for (auto& val : e)
            {
                if (val.name() == "a_a" || val.name() == "a_c")
                {
                    ++no_vals;
                    REQUIRE(!val.value());
                }
                else if (val.name() == "a_b")
                {
                    ++no_vals;
                    REQUIRE(val.value());
                    auto& expr = val.value().value();
                    if (equal_types(idx, expr.type(), *cpp_builtin_type::build(cpp_uint)))
                    {
                        REQUIRE(expr.kind() == cpp_expression_kind::unexposed_t);
                        REQUIRE(static_cast<const cpp_unexposed_expression&>(expr)
                                    .expression()
                                    .as_string()
                                == "42");
                    }
                    else
                    {
                        REQUIRE(equal_types(idx, expr.type(), *cpp_builtin_type::build(cpp_int)));
                        REQUIRE(expr.kind() == cpp_expression_kind::literal_t);
                        REQUIRE(static_cast<const cpp_literal_expression&>(expr).value() == "42");
                    }
                }
                else if (val.name() == "a_d")
                {
                    ++no_vals;
                    REQUIRE(val.value());
                    auto& expr = val.value().value();
                    REQUIRE(expr.kind() == cpp_expression_kind::unexposed_t);
                    REQUIRE(
                        static_cast<const cpp_unexposed_expression&>(expr).expression().as_string()
                        == "a_a+2");
                    if (!equal_types(idx, expr.type(), *cpp_builtin_type::build(cpp_int)))
                        REQUIRE(equal_types(idx, expr.type(), *cpp_builtin_type::build(cpp_uint)));
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(no_vals == 4u);
        }
        else if (e.name() == "b")
        {
            REQUIRE(e.is_scoped());

            if (e.is_definition())
            {
                REQUIRE(e.has_explicit_type());
                REQUIRE(equal_types(idx, e.underlying_type(), *cpp_builtin_type::build(cpp_int)));

                auto no_vals = 0u;
                for (auto& val : e)
                {
                    REQUIRE(full_name(val) == "b::" + val.name());
                    if (val.name() == "b_a" || val.name() == "b_c")
                    {
                        ++no_vals;
                        REQUIRE(!val.value());
                    }
                    else if (val.name() == "b_b")
                    {
                        ++no_vals;
                        REQUIRE(val.value());
                        auto& expr = val.value().value();
                        REQUIRE(expr.kind() == cpp_expression_kind::literal_t);
                        REQUIRE(static_cast<const cpp_literal_expression&>(expr).value() == "42");
                        REQUIRE(equal_types(idx, expr.type(), *cpp_builtin_type::build(cpp_int)));
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(no_vals == 3u);
            }
            else if (e.is_declaration())
            {
                REQUIRE(!e.has_explicit_type()); // no underlying type, implicit int
                REQUIRE(count_children(e) == 0u);

                auto definition = get_definition(idx, e);
                REQUIRE(definition);
                REQUIRE(definition.value().name() == "b");
                REQUIRE(count_children(definition.value()) == 3u);
            }
            else
                REQUIRE(false);
        }
        else if (e.name() == "c")
        {
            if (e.semantic_scope() == "ns::")
                REQUIRE(e.is_definition());
            else
                REQUIRE(e.is_declaration());

            REQUIRE(!e.is_scoped());
            REQUIRE(e.has_explicit_type());
            REQUIRE(equal_types(idx, e.underlying_type(), *cpp_builtin_type::build(cpp_int)));
            REQUIRE(count_children(e) == 0u);

            auto definition = get_definition(idx, e);
            REQUIRE(definition);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);
}
