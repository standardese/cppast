// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_variable.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_variable")
{
    auto code = R"(
// basic - global scope, so external storage
int a;
unsigned long long b = 42;
float c = 3.f + 0.14f;

// with storage class specifiers
extern int d; // actually declaration
static int e;
thread_local int f;
thread_local static int g;

// constexpr
constexpr int h = 12;
)";

    cpp_entity_index idx;
    auto check_variable = [&](const cpp_variable& var, const cpp_type& type,
                              type_safe::optional_ref<const cpp_expression> default_value,
                              int storage_class, bool is_constexpr) {
        REQUIRE(equal_types(idx, var.type(), type));
        if (var.default_value())
        {
            REQUIRE(default_value);
            REQUIRE(equal_expressions(var.default_value().value(), default_value.value()));
        }
        else
            REQUIRE(!default_value);

        REQUIRE(var.storage_class() == storage_class);
        REQUIRE(var.is_constexpr() == is_constexpr);
    };

    auto file = parse({}, "cpp_variable.cpp", code);

    auto int_type = cpp_builtin_type::build("int");
    auto count    = test_visit<cpp_variable>(*file, [&](const cpp_variable& var) {
        INFO(var.name());
        if (var.name() == "a")
            check_variable(var, *int_type, nullptr, cpp_storage_class_none, false);
        else if (var.name() == "b")
            check_variable(var, *cpp_builtin_type::build("unsigned long long"),
                           // unexposed due to implicit cast, I think
                           *cpp_unexposed_expression::build(cpp_builtin_type::build("int"), "42"),
                           cpp_storage_class_none, false);
        else if (var.name() == "c")
            check_variable(var, *cpp_builtin_type::build("float"),
                           *cpp_unexposed_expression::build(cpp_builtin_type::build("float"),
                                                            "3.f+0.14f"),
                           cpp_storage_class_none, false);
        else if (var.name() == "d")
            check_variable(var, *int_type, nullptr, cpp_storage_class_extern, false);
        else if (var.name() == "e")
            check_variable(var, *int_type, nullptr, cpp_storage_class_static, false);
        else if (var.name() == "f")
            check_variable(var, *int_type, nullptr, cpp_storage_class_thread_local, false);
        else if (var.name() == "g")
            check_variable(var, *int_type, nullptr,
                           cpp_storage_class_static | cpp_storage_class_thread_local, false);
        else if (var.name() == "h")
            check_variable(var, *cpp_cv_qualified_type::build(cpp_builtin_type::build("int"),
                                                              cpp_cv_const),
                           *cpp_literal_expression::build(cpp_builtin_type::build("int"), "12"),
                           cpp_storage_class_none, true);
        else
            REQUIRE(false);
    });
    REQUIRE(count == 8u);
}
