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

TEST_CASE("cpp_conversion_op")
{
    auto code = R"(
namespace ns
{
    using type = char;
}

// most of it only need to be check in member function
struct foo
{
    operator int&();
    explicit operator bool() const;
    constexpr operator ns::type();
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_conversion_op.cpp", code);
    auto count            = test_visit<cpp_conversion_op>(*file, [&](const cpp_conversion_op& op) {
        REQUIRE(op.name().empty());
        REQUIRE(count_children(op) == 0u);
        REQUIRE(!op.is_variadic());
        REQUIRE(op.body_kind() == cpp_function_declaration);
        REQUIRE(op.ref_qualifier() == cpp_ref_none);
        REQUIRE(!op.virtual_info());
        REQUIRE(!op.noexcept_condition());

        if (!op.is_explicit() && !op.is_constexpr())
        {
            REQUIRE(equal_types(idx, op.return_type(),
                                *cpp_reference_type::build(cpp_builtin_type::build("int"),
                                                           cpp_ref_lvalue)));
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
            REQUIRE(!op.is_explicit());
            REQUIRE(!op.is_constexpr());
        }
        else if (op.is_explicit() && !op.is_constexpr())
        {
            REQUIRE(equal_types(idx, op.return_type(), *cpp_builtin_type::build("bool")));
            REQUIRE(op.cv_qualifier() == cpp_cv_const);
        }
        else if (!op.is_explicit() && op.is_constexpr())
        {
            REQUIRE(equal_types(idx, op.return_type(),
                                *cpp_user_defined_type::build(
                                    cpp_type_ref(cpp_entity_id(""), "ns::type"))));
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}
