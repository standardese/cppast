// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_template.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_member_function")
{
    auto code = R"(
// no need to test parameters/return types
template <typename T>
struct foo
{
    /// void a();
    void a();
    /// void b()noexcept;
    void b() noexcept;

    /// void c()const;
    void c() const;
    /// void d()const volatile noexcept;
    auto d() const volatile noexcept -> void;
    /// void e()&;
    void e() &;
    /// void f()const volatile&&;
    void f() const volatile &&;

    /// virtual void g();
    virtual void g();
    /// virtual void h()=0;
    virtual void h() = 0;

    /// void i();
    void i() {}
    /// void j()=delete;
    void j() = delete;
};

/// void foo<T>::a();
template <typename T>
void foo<T>::a() {}

struct bar : foo<int>
{
    /// virtual void g() override;
    void g();
    /// virtual void h() override final;
    virtual auto h() -> void override final;
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_member_function.cpp", code);
    auto count = test_visit<cpp_member_function>(*file, [&](const cpp_member_function& func) {
        REQUIRE(count_children(func.parameters()) == 0u);
        REQUIRE(!func.is_variadic());
        REQUIRE(!func.is_constexpr());
        REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_void)));
        if (func.name() != "b" && func.name() != "d")
            REQUIRE(!func.noexcept_condition());
        if (func.name() != "g" && func.name() != "h")
            REQUIRE(!func.virtual_info());
        if (func.semantic_scope().empty() && func.name() != "i" && func.name() != "j")
            REQUIRE(func.body_kind() == cpp_function_declaration);

        if (func.name() == "a")
        {
            if (func.semantic_scope() == "foo::")
                REQUIRE(func.is_definition());
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
        }
        else if (func.name() == "b")
        {
            REQUIRE(func.noexcept_condition());
            REQUIRE(
                equal_expressions(func.noexcept_condition().value(),
                                  *cpp_literal_expression::build(cpp_builtin_type::build(cpp_bool),
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
    REQUIRE(count == 13u);
}

TEST_CASE("cpp_conversion_op")
{
    auto code = R"(
namespace ns
{
    template <typename T>
    using type = char;
}

// most of it only need to be check in member function
struct foo
{
    /// operator int&();
    operator int&();
    /// explicit operator bool()const;
    explicit operator bool() const;
    /// constexpr operator ns::type<int>();
    constexpr operator ns::type<int>();
};
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_conversion_op.cpp", code);
    auto count            = test_visit<cpp_conversion_op>(*file, [&](const cpp_conversion_op& op) {
        REQUIRE(count_children(op.parameters()) == 0u);
        REQUIRE(!op.is_variadic());
        REQUIRE(op.body_kind() == cpp_function_declaration);
        REQUIRE(op.ref_qualifier() == cpp_ref_none);
        REQUIRE(!op.virtual_info());
        REQUIRE(!op.noexcept_condition());

        if (!op.is_explicit() && !op.is_constexpr())
        {
            REQUIRE(op.name() == "operator int&");
            REQUIRE(equal_types(idx, op.return_type(),
                                *cpp_reference_type::build(cpp_builtin_type::build(cpp_int),
                                                           cpp_ref_lvalue)));
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
        }
        else if (op.is_explicit() && !op.is_constexpr())
        {
            REQUIRE(op.name() == "operator bool");
            REQUIRE(equal_types(idx, op.return_type(), *cpp_builtin_type::build(cpp_bool)));
            REQUIRE(op.cv_qualifier() == cpp_cv_const);
        }
        else if (!op.is_explicit() && op.is_constexpr())
        {
            REQUIRE(op.name() == "operator ns::type<int>");
            cpp_template_instantiation_type::builder builder(
                cpp_template_ref(cpp_entity_id(""), "ns::type"));
            builder.add_unexposed_arguments("int");
            REQUIRE(equal_types(idx, op.return_type(), *builder.finish()));
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 3u);
}

TEST_CASE("cpp_constructor")
{
    // only test constructor specific stuff
    const char* code;
    auto        is_template = false;
    SECTION("non-template")
    {
        code = R"(
struct foo
{
    /// foo()noexcept=default;
    foo() noexcept = default;
    /// explicit foo(int);
    explicit foo(int);
    /// constexpr foo(int,char)=delete;
    constexpr foo(int, char) = delete;
};

/// foo::foo(int);
foo::foo(int) {}
)";
    }
    SECTION("template")
    {
        is_template = true;
        code        = R"(
template <typename T>
struct foo
{
    /// foo()noexcept=default;
    foo() noexcept = default;
    /// explicit foo(int);
    explicit foo(int);
    /// constexpr foo(int,char)=delete;
    constexpr foo(int, char) = delete;
};

/// foo<T>::foo(int);
template <typename T>
foo<T>::foo(int) {}
)";
    }

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_constructor.cpp", code);
    auto count            = test_visit<cpp_constructor>(*file, [&](const cpp_constructor& cont) {
        REQUIRE(!cont.is_variadic());
        REQUIRE(cont.name() == "foo");

        if (cont.semantic_parent())
        {
            if (is_template)
                REQUIRE(cont.semantic_parent().value().name() == "foo<T>::");
            else
                REQUIRE(cont.semantic_parent().value().name() == "foo::");
            REQUIRE(!cont.noexcept_condition());
            REQUIRE(!cont.is_constexpr());
        }
        else
        {
            REQUIRE(cont.name() == "foo");

            if (count_children(cont.parameters()) == 0u)
            {
                REQUIRE(cont.noexcept_condition());
                REQUIRE(equal_expressions(cont.noexcept_condition().value(),
                                          *cpp_literal_expression::build(cpp_builtin_type::build(
                                                                             cpp_bool),
                                                                         "true")));
                REQUIRE(!cont.is_explicit());
                REQUIRE(!cont.is_constexpr());
                REQUIRE(cont.body_kind() == cpp_function_defaulted);
            }
            else if (count_children(cont.parameters()) == 1u)
            {
                REQUIRE(!cont.noexcept_condition());
                REQUIRE(cont.is_explicit());
                REQUIRE(!cont.is_constexpr());
                REQUIRE(cont.body_kind() == cpp_function_declaration);
            }
            else if (count_children(cont.parameters()) == 2u)
            {
                REQUIRE(!cont.noexcept_condition());
                REQUIRE(!cont.is_explicit());
                REQUIRE(cont.is_constexpr());
                REQUIRE(cont.body_kind() == cpp_function_deleted);
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 4u);
}

TEST_CASE("cpp_destructor")
{
    auto code = R"(
struct a
{
    /// ~a();
    ~a();
};

struct b
{
    /// ~b()noexcept(false);
    ~b() noexcept(false) {}
};

struct c
{
    /// virtual ~c()=default;
    virtual ~c() = default;
};

struct d : c
{
    /// virtual ~d() override final;
    ~d() final;
};
)";

    auto file  = parse({}, "cpp_destructor.cpp", code);
    auto count = test_visit<cpp_destructor>(*file, [&](const cpp_destructor& dtor) {
        REQUIRE(count_children(dtor.parameters()) == 0u);
        REQUIRE(!dtor.is_variadic());

        if (dtor.name() == "~a")
        {
            REQUIRE(!dtor.is_virtual());
            REQUIRE(dtor.is_declaration());
            REQUIRE(!dtor.noexcept_condition());
        }
        else if (dtor.name() == "~b")
        {
            REQUIRE(!dtor.is_virtual());
            REQUIRE(dtor.body_kind() == cpp_function_definition);
            REQUIRE(dtor.noexcept_condition());
            REQUIRE(equal_expressions(dtor.noexcept_condition().value(),
                                      *cpp_unexposed_expression::build(cpp_builtin_type::build(
                                                                           cpp_bool),
                                                                       "false")));
        }
        else if (dtor.name() == "~c")
        {
            REQUIRE(dtor.virtual_info());
            REQUIRE(dtor.virtual_info().value() == type_safe::flag_set<cpp_virtual_flags>{});
            REQUIRE(dtor.body_kind() == cpp_function_defaulted);
            REQUIRE(!dtor.noexcept_condition());
        }
        else if (dtor.name() == "~d")
        {
            REQUIRE(dtor.virtual_info());
            REQUIRE(dtor.virtual_info().value()
                    == (cpp_virtual_flags::override | cpp_virtual_flags::final));
            REQUIRE(!dtor.noexcept_condition());
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 4u);
}
