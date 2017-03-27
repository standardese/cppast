// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_function.hpp>
#include <cppast/cpp_array_type.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_function")
{
    auto code = R"(
// parameters and return type are only tested here
void a();
int b(int a, float* b = nullptr);
int (&c(int a, ...))[10];

// noexcept conditions
void d() noexcept;
void e() noexcept(false);
void f() noexcept(noexcept(d()));

// storage class + constexpr
extern void g();
static void h();
constexpr void i();
static constexpr void j();

// body
namespace ns
{
    void k() = delete;

    void l();
}

void ns::l()
{
    // might confuse parser
    auto b = noexcept(g());
}
)";

    auto check_body = [](const cpp_function& func, cpp_function_body_kind kind) {
        REQUIRE(func.body_kind() == kind);
        REQUIRE(func.is_declaration() == is_declaration(kind));
        REQUIRE(func.is_definition() == is_definition(kind));
    };

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_function.cpp", code);
    auto count            = test_visit<cpp_function>(*file, [&](const cpp_function& func) {
        REQUIRE(!func.is_friend());

        if (func.name() == "a" || func.name() == "b" || func.name() == "c")
        {
            REQUIRE(!func.noexcept_condition());
            REQUIRE(func.storage_class() == cpp_storage_class_none);
            REQUIRE(!func.is_constexpr());
            check_body(func, cpp_function_declaration);

            if (func.name() == "a")
            {
                REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
                REQUIRE(count_children(func) == 0u);
                REQUIRE(!func.is_variadic());
            }
            else if (func.name() == "b")
            {
                REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("int")));

                auto count = 0u;
                for (auto& param : func)
                {
                    if (param.name() == "a")
                    {
                        REQUIRE(equal_types(idx, param.type(), *cpp_builtin_type::build("int")));
                        REQUIRE(!param.default_value());
                    }
                    else if (param.name() == "b")
                    {
                        REQUIRE(
                            equal_types(idx, param.type(), *cpp_pointer_type::build(
                                                               cpp_builtin_type::build("float"))));
                        REQUIRE(param.default_value());
                        REQUIRE(equal_expressions(param.default_value().value(),
                                                  *cpp_unexposed_expression::
                                                      build(cpp_pointer_type::build(
                                                                cpp_builtin_type::build("float")),
                                                            "nullptr")));
                    }
                    else
                        REQUIRE(false);
                    ++count;
                }
                REQUIRE(count == 2u);
                REQUIRE(!func.is_variadic());
            }
            else if (func.name() == "c")
            {
                REQUIRE(
                    equal_types(idx, func.return_type(),
                                *cpp_reference_type::
                                    build(cpp_array_type::build(cpp_builtin_type::build("int"),
                                                                cpp_literal_expression::
                                                                    build(cpp_builtin_type::build(
                                                                              "unsigned long long"),
                                                                          "10")),
                                          cpp_ref_lvalue)));

                auto count = 0u;
                for (auto& param : func)
                {
                    if (param.name() == "a")
                    {
                        REQUIRE(equal_types(idx, param.type(), *cpp_builtin_type::build("int")));
                        REQUIRE(!param.default_value());
                    }
                    else
                        REQUIRE(false);
                    ++count;
                }
                REQUIRE(count == 1u);
                REQUIRE(func.is_variadic());
            }
        }
        else if (func.name() == "d" || func.name() == "e" || func.name() == "f")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
            REQUIRE(count_children(func) == 0u);
            REQUIRE(!func.is_variadic());
            REQUIRE(func.storage_class() == cpp_storage_class_none);
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.noexcept_condition());
            check_body(func, cpp_function_declaration);

            auto bool_t = cpp_builtin_type::build("bool");
            if (func.name() == "d")
                REQUIRE(
                    equal_expressions(func.noexcept_condition().value(),
                                      *cpp_literal_expression::build(std::move(bool_t), "true")));
            else if (func.name() == "e")
                REQUIRE(equal_expressions(func.noexcept_condition().value(),
                                          *cpp_unexposed_expression::build(std::move(bool_t),
                                                                           "false")));
            else if (func.name() == "f")
                REQUIRE(equal_expressions(func.noexcept_condition().value(),
                                          *cpp_unexposed_expression::build(std::move(bool_t),
                                                                           "noexcept(d())")));
        }
        else if (func.name() == "g" || func.name() == "h" || func.name() == "i"
                 || func.name() == "j")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
            REQUIRE(count_children(func) == 0u);
            REQUIRE(!func.is_variadic());
            REQUIRE(!func.noexcept_condition());
            check_body(func, cpp_function_declaration);

            if (func.name() == "g")
            {
                REQUIRE(!func.is_constexpr());
                REQUIRE(func.storage_class() == cpp_storage_class_extern);
            }
            else if (func.name() == "h")
            {
                REQUIRE(!func.is_constexpr());
                REQUIRE(func.storage_class() == cpp_storage_class_static);
            }
            else if (func.name() == "i")
            {
                REQUIRE(func.is_constexpr());
                REQUIRE(func.storage_class() == cpp_storage_class_none);
            }
            else if (func.name() == "j")
            {
                REQUIRE(func.is_constexpr());
                REQUIRE(func.storage_class() == cpp_storage_class_static);
            }
        }
        else if (func.name() == "k" || func.name() == "l" || func.name() == "ns::l")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
            REQUIRE(count_children(func) == 0u);
            REQUIRE(!func.is_variadic());
            REQUIRE(!func.noexcept_condition());
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.storage_class() == cpp_storage_class_none);

            if (func.name() == "k")
                check_body(func, cpp_function_deleted);
            else if (func.name() == "l")
                check_body(func, cpp_function_declaration);
            else if (func.name() == "ns::l")
                check_body(func, cpp_function_definition);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 13u);
}

TEST_CASE("static cpp_function")
{
    auto code = R"(
// no need to test anything special
struct foo
{
    static void a();

    static int b() noexcept { return 0; }

    static constexpr char c() = delete;
};

void foo::a() {}
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "static_cpp_function.cpp", code);
    auto count            = test_visit<cpp_function>(*file, [&](const cpp_function& func) {
        REQUIRE(!func.is_variadic());
        REQUIRE(count_children(func) == 0u);
        REQUIRE(func.storage_class() == cpp_storage_class_static);

        if (func.name() == "a" || func.name() == "foo::a")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
            REQUIRE(!func.noexcept_condition());
            REQUIRE(!func.is_constexpr());

            if (func.name() == "a")
                REQUIRE(func.body_kind() == cpp_function_declaration);
            else
                REQUIRE(func.body_kind() == cpp_function_definition);
        }
        else if (func.name() == "b")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("int")));
            REQUIRE(func.noexcept_condition());
            REQUIRE(
                equal_expressions(func.noexcept_condition().value(),
                                  *cpp_literal_expression::build(cpp_builtin_type::build("bool"),
                                                                 "true")));
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.body_kind() == cpp_function_definition);
        }
        else if (func.name() == "c")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("char")));
            REQUIRE(!func.noexcept_condition());
            REQUIRE(func.is_constexpr());
            REQUIRE(func.body_kind() == cpp_function_deleted);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 4u);
}

// TODO: friend functions (clang 4.0 required)
