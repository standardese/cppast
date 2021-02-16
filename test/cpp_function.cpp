// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_function.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_decltype_type.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_function")
{
    auto code = R"(
// parameters and return type are only tested here
/// void a();
void a();
/// int b(int a,float* b=nullptr);
int b(int a, float* b = nullptr);
/// auto c(decltype(42) a,...)->int(&)[10];
int (&c(decltype(42) a, ...))[10];

// noexcept conditions
/// void d()noexcept;
void d() noexcept;
/// void e()noexcept(false);
void e() noexcept(false);
/// void f()noexcept(noexcept(d()));
void f() noexcept(noexcept(d()));

// storage class + constexpr
/// extern void g();
extern void g();
/// static void h();
static void h();
/// constexpr void i();
constexpr void i();
/// static constexpr void j();
static constexpr void j();

// body
namespace ns
{
    /// void k()=delete;
    void k() = delete;

    /// void l();
    void l();

    using m = int;
}

/// void ns::l();
void ns::l()
{
    // might confuse parser
    auto b = noexcept(g());
}

/// ns::m m();
ns::m m();

/// void n(int i=int());
void n(int i = int());
)";

    auto check_body = [](const cpp_function& func, cpp_function_body_kind kind) {
        REQUIRE(func.body_kind() == kind);
        REQUIRE(func.is_declaration() == is_declaration(kind));
        REQUIRE(func.is_definition() == is_definition(kind));
    };

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_function.cpp", code);
    auto             count = test_visit<cpp_function>(*file, [&](const cpp_function& func) {
        if (func.name() == "a" || func.name() == "b" || func.name() == "c" || func.name() == "n")
        {
            REQUIRE(!func.noexcept_condition());
            REQUIRE(func.storage_class() == cpp_storage_class_none);
            REQUIRE(!func.is_constexpr());
            check_body(func, cpp_function_declaration);

            if (func.name() == "a")
            {
                REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
                REQUIRE(func.signature() == "()");
                REQUIRE(!func.is_variadic());
            }
            else if (func.name() == "b")
            {
                REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_int)));
                REQUIRE(func.signature() == "(int,float*)");

                auto count = 0u;
                for (auto& param : func.parameters())
                {
                    if (param.name() == "a")
                    {
                        REQUIRE(equal_types(idx, param.type(), *cpp_builtin_type::build(cpp_int)));
                        REQUIRE(!param.default_value());
                    }
                    else if (param.name() == "b")
                    {
                        REQUIRE(equal_types(idx, param.type(),
                                            *cpp_pointer_type::build(
                                                cpp_builtin_type::build(cpp_float))));
                        REQUIRE(param.default_value());
                        REQUIRE(
                            equal_expressions(param.default_value().value(),
                                              *cpp_unexposed_expression::
                                                  build(cpp_pointer_type::build(
                                                            cpp_builtin_type::build(cpp_float)),
                                                        cpp_token_string::tokenize("nullptr"))));
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
                                    build(cpp_array_type::build(cpp_builtin_type::build(cpp_int),
                                                                cpp_literal_expression::
                                                                    build(cpp_builtin_type::build(
                                                                              cpp_ulonglong),
                                                                          "10")),
                                          cpp_ref_lvalue)));
                REQUIRE(func.signature() == "(decltype(42),...)");

                auto count = 0u;
                for (auto& param : func.parameters())
                {
                    if (param.name() == "a")
                    {
                        REQUIRE(equal_types(idx, param.type(),
                                            *cpp_decltype_type::build(
                                                cpp_unexposed_expression::
                                                    build(cpp_builtin_type::build(cpp_int),
                                                          cpp_token_string::tokenize("42")))));
                        REQUIRE(!param.default_value());
                    }
                    else
                        REQUIRE(false);
                    ++count;
                }
                REQUIRE(count == 1u);
                REQUIRE(func.is_variadic());
            }
            else if (func.name() == "n")
            {
                REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
                REQUIRE(func.signature() == "(int)");

                auto count = 0u;
                for (auto& param : func.parameters())
                {
                    if (param.name() == "i")
                    {
                        REQUIRE(equal_types(idx, param.type(), *cpp_builtin_type::build(cpp_int)));
                        REQUIRE(param.default_value());
                        REQUIRE(equal_expressions(param.default_value().value(),
                                                  *cpp_unexposed_expression::
                                                      build(cpp_pointer_type::build(
                                                                cpp_builtin_type::build(cpp_int)),
                                                            cpp_token_string::tokenize("int()"))));
                    }
                    else
                        REQUIRE(false);
                    ++count;
                }
                REQUIRE(count == 1u);
                REQUIRE(!func.is_variadic());
            }
        }
        else if (func.name() == "d" || func.name() == "e" || func.name() == "f")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
            REQUIRE(func.signature() == "()");
            REQUIRE(!func.is_variadic());
            REQUIRE(func.storage_class() == cpp_storage_class_none);
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.noexcept_condition());
            check_body(func, cpp_function_declaration);

            auto bool_t = cpp_builtin_type::build(cpp_bool);
            if (func.name() == "d")
                REQUIRE(
                    equal_expressions(func.noexcept_condition().value(),
                                      *cpp_literal_expression::build(std::move(bool_t), "true")));
            else if (func.name() == "e")
                REQUIRE(
                    equal_expressions(func.noexcept_condition().value(),
                                      *cpp_unexposed_expression::build(std::move(bool_t),
                                                                       cpp_token_string::tokenize(
                                                                           "false"))));
            else if (func.name() == "f")
                REQUIRE(
                    equal_expressions(func.noexcept_condition().value(),
                                      *cpp_unexposed_expression::build(std::move(bool_t),
                                                                       cpp_token_string::tokenize(
                                                                           "noexcept(d())"))));
        }
        else if (func.name() == "g" || func.name() == "h" || func.name() == "i"
                 || func.name() == "j")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
            REQUIRE(func.signature() == "()");
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
        else if (func.name() == "k" || func.name() == "l")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
            REQUIRE(func.signature() == "()");
            REQUIRE(!func.is_variadic());
            REQUIRE(!func.noexcept_condition());
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.storage_class() == cpp_storage_class_none);

            if (func.name() == "k")
                check_body(func, cpp_function_deleted);
            else if (func.name() == "l")
            {
                if (func.semantic_scope() == "ns::")
                    check_body(func, cpp_function_definition);
                else
                    check_body(func, cpp_function_declaration);
            }
        }
        else if (func.name() == "m")
        {
            REQUIRE(equal_types(idx, func.return_type(),
                                *cpp_user_defined_type::build(
                                    cpp_type_ref(cpp_entity_id(""), "ns::m"))));
            REQUIRE(count_children(func.parameters()) == 0u);
            REQUIRE(!func.is_variadic());
            REQUIRE(!func.noexcept_condition());
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.storage_class() == cpp_storage_class_none);
            check_body(func, cpp_function_declaration);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 15u);
}

TEST_CASE("consteval cpp_function")
{
    if (libclang_parser::libclang_minor_version() < 60)
        return;

    auto code       = R"(
/// consteval void p();
consteval void p();
/// static consteval void q();
static consteval void q();
)";
    auto check_body = [](const cpp_function& func, cpp_function_body_kind kind) {
        REQUIRE(func.body_kind() == kind);
        REQUIRE(func.is_declaration() == is_declaration(kind));
        REQUIRE(func.is_definition() == is_definition(kind));
    };

    cpp_entity_index idx;
    auto file  = parse(idx, "consteval_function.cpp", code, false, cppast::cpp_standard::cpp_2a);
    auto count = test_visit<cpp_function>(*file, [&](const cpp_function& func) {
        if (func.name() == "p" || func.name() == "q")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
            REQUIRE(func.signature() == "()");
            REQUIRE(!func.is_variadic());
            REQUIRE(!func.noexcept_condition());
            check_body(func, cpp_function_declaration);

            if (func.name() == "p")
            {
                REQUIRE(func.is_consteval());
                REQUIRE(func.storage_class() == cpp_storage_class_none);
            }
            else if (func.name() == "q")
            {
                REQUIRE(func.is_consteval());
                REQUIRE(func.storage_class() == cpp_storage_class_static);
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 2u);
}

TEST_CASE("static cpp_function")
{
    auto code = R"(
// no need to test anything special
struct foo
{
    /// static void a();
    static void a();

    /// static int b()noexcept;
    static int b() noexcept { return 0; }

    /// static constexpr char c()=delete;
    static constexpr char c() = delete;
};

/// static void foo::a();
void foo::a() {}
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "static_cpp_function.cpp", code);
    auto             count = test_visit<cpp_function>(*file, [&](const cpp_function& func) {
        REQUIRE(!func.is_variadic());
        REQUIRE(func.signature() == "()");
        REQUIRE(func.storage_class() == cpp_storage_class_static);

        if (func.name() == "a")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
            REQUIRE(!func.noexcept_condition());
            REQUIRE(!func.is_constexpr());

            if (func.semantic_scope() == "foo::")
                REQUIRE(func.body_kind() == cpp_function_definition);
            else
                REQUIRE(func.body_kind() == cpp_function_declaration);
        }
        else if (func.name() == "b")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_int)));
            REQUIRE(func.noexcept_condition());
            REQUIRE(
                equal_expressions(func.noexcept_condition().value(),
                                  *cpp_literal_expression::build(cpp_builtin_type::build(cpp_bool),
                                                                 "true")));
            REQUIRE(!func.is_constexpr());
            REQUIRE(func.body_kind() == cpp_function_definition);
        }
        else if (func.name() == "c")
        {
            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_char)));
            REQUIRE(!func.noexcept_condition());
            REQUIRE(func.is_constexpr());
            REQUIRE(func.body_kind() == cpp_function_deleted);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 4u);
}
