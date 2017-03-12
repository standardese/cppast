// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_member_function.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_member_function")
{
    auto code = R"(
// no need to test parameters/return types
struct foo
{
    void a();
    void b() noexcept;

    void c() const;
    auto d() const volatile -> void;
    void e() &;
    void f() const volatile &&;

    virtual void g();
    virtual void h() = 0;

    void i() {}
    void j() = delete;
};

struct bar : foo
{
    void g();
    virtual auto h() -> void override final;
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_member_function.cpp", code);
    auto count = test_visit<cpp_member_function>(*file, [&](const cpp_member_function& func) {
        REQUIRE(count_children(func) == 0u);
        REQUIRE(!func.is_variadic());
        REQUIRE(!func.is_constexpr());
        REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build("void")));
        if (func.name() != "b")
            REQUIRE(!func.noexcept_condition());
        if (func.name() != "g" && func.name() != "h")
            REQUIRE(!func.virtual_info());
        if (func.name() != "i" && func.name() != "j")
            REQUIRE(func.body_kind() == cpp_function_declaration);

        if (func.name() == "a")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
        }
        else if (func.name() == "b")
        {
            REQUIRE(func.noexcept_condition());
            REQUIRE(
                equal_expressions(func.noexcept_condition().value(),
                                  *cpp_literal_expression::build(cpp_builtin_type::build("bool"),
                                                                 "true")));
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
        }
        else if (func.name() == "c")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
        }
        else if (func.name() == "d")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const_volatile);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
        }
        else if (func.name() == "e")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_lvalue);
        }
        else if (func.name() == "f")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const_volatile);
            REQUIRE(func.ref_qualifier() == cpp_ref_rvalue);
        }
        else if (func.name() == "g")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.virtual_info());
            if (func.parent().value().name() == "foo")
                REQUIRE(func.virtual_info().value() == type_safe::flag_set<cpp_virtual_flags>{});
            else if (func.parent().value().name() == "bar")
            {
                INFO(bool(func.virtual_info().value() & cpp_virtual_flags::override));
                REQUIRE(func.virtual_info().value() == cpp_virtual_flags::override);
            }
            else
                REQUIRE(false);
        }
        else if (func.name() == "h")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.virtual_info());
            if (func.parent().value().name() == "foo")
                REQUIRE(func.virtual_info().value() == cpp_virtual_flags::pure);
            else if (func.parent().value().name() == "bar")
                REQUIRE(func.virtual_info().value()
                        == (cpp_virtual_flags::override | cpp_virtual_flags::final));
            else
                REQUIRE(false);
        }
        else if (func.name() == "i")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.body_kind() == cpp_function_definition);
        }
        else if (func.name() == "j")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.body_kind() == cpp_function_deleted);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 12u);
}
