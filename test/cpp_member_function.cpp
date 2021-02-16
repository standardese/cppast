// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "test_parser.hpp"
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_template.hpp>

using namespace cppast;

TEST_CASE("cpp_member_function")
{
    auto code = R"(
// no need to test parameters/return types
template <typename T>
struct foo
{
    /// void a(int array[]);
    void a(int array[]); // throw in an array argument for good measure
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

/// void foo<T>::a(int array[]);
template <typename T>
void foo<T>::a(int array[]) {}

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
            REQUIRE(func.signature() == "(int[])");
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
            REQUIRE(func.signature() == "()");
        }
        else if (func.name() == "c")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.signature() == "() const");
        }
        else if (func.name() == "d")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const_volatile);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.signature() == "() const volatile");
        }
        else if (func.name() == "e")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_lvalue);
            REQUIRE(func.signature() == "() &");
        }
        else if (func.name() == "f")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_const_volatile);
            REQUIRE(func.ref_qualifier() == cpp_ref_rvalue);
            REQUIRE(func.signature() == "() const volatile &&");
        }
        else if (func.name() == "g")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.signature() == "()");
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
            REQUIRE(func.signature() == "()");
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
            REQUIRE(func.signature() == "()");
            REQUIRE(func.body_kind() == cpp_function_definition);
        }
        else if (func.name() == "j")
        {
            REQUIRE(func.cv_qualifier() == cpp_cv_none);
            REQUIRE(func.ref_qualifier() == cpp_ref_none);
            REQUIRE(func.signature() == "()");
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
    struct type {};
}

// most of it only need to be check in member function
struct foo
{
    /// operator int&();
    operator int&()
    {
        static int i;
        return i;
    }

    /// explicit operator bool()const;
    explicit operator bool() const
    {
        return false;
    }

    /// constexpr operator ns::type<int>();
    constexpr operator ns::type<int>()
    {
        return {};
    }

    /// constexpr operator ns::type<char>();
    constexpr operator ns::type<char>()
    {
        return {};
    }
};
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_conversion_op.cpp", code);
    auto             count = test_visit<cpp_conversion_op>(*file, [&](const cpp_conversion_op& op) {
        REQUIRE(count_children(op.parameters()) == 0u);
        REQUIRE(!op.is_variadic());
        REQUIRE(op.body_kind() == cpp_function_definition);
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
            REQUIRE(op.signature() == "()");
        }
        else if (op.is_explicit() && !op.is_constexpr())
        {
            REQUIRE(op.name() == "operator bool");
            REQUIRE(equal_types(idx, op.return_type(), *cpp_builtin_type::build(cpp_bool)));
            REQUIRE(op.cv_qualifier() == cpp_cv_const);
            REQUIRE(op.signature() == "() const");
        }
        else if (!op.is_explicit() && op.is_constexpr())
        {
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
            REQUIRE(op.signature() == "()");
            if (op.name() == "operator ns::type<int>")
            {
                REQUIRE(op.return_type().kind() == cpp_type_kind::template_instantiation_t);
                auto& inst = static_cast<const cpp_template_instantiation_type&>(op.return_type());

                REQUIRE(inst.primary_template().name() == "ns::type");
                REQUIRE(!inst.arguments_exposed());
                REQUIRE(inst.unexposed_arguments() == "int");
            }
            else if (op.name() == "operator ns::type<char>")
            {
                REQUIRE(op.return_type().kind() == cpp_type_kind::template_instantiation_t);
                auto& inst = static_cast<const cpp_template_instantiation_type&>(op.return_type());

                REQUIRE(inst.primary_template().name() == "ns::type");
                REQUIRE(!inst.arguments_exposed());
                REQUIRE(inst.unexposed_arguments() == "char");
            }
            else
                REQUIRE(false);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 4u);
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
; // there's a bug on MSVC's libclang, we have to give it a semicolon
)";
    }
    INFO(is_template);

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_constructor.cpp", code);
    auto             count = test_visit<cpp_constructor>(*file, [&](const cpp_constructor& cont) {
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
                REQUIRE(cont.signature() == "()");
            }
            else if (count_children(cont.parameters()) == 1u)
            {
                REQUIRE(!cont.noexcept_condition());
                REQUIRE(cont.is_explicit());
                REQUIRE(!cont.is_constexpr());
                REQUIRE(cont.body_kind() == cpp_function_declaration);
                REQUIRE(cont.signature() == "(int)");
            }
            else if (count_children(cont.parameters()) == 2u)
            {
                REQUIRE(!cont.noexcept_condition());
                REQUIRE(!cont.is_explicit());
                REQUIRE(cont.is_constexpr());
                REQUIRE(cont.body_kind() == cpp_function_deleted);
                REQUIRE(cont.signature() == "(int,char)");
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

/// virtual d::~d();
d::~d() {}

struct e : c
{
    /// virtual ~e() override final;
    ~e() final;
};

/// virtual e::~e()=default;
e::~e() = default;

)";

    auto file  = parse({}, "cpp_destructor.cpp", code);
    auto count = test_visit<cpp_destructor>(*file, [&](const cpp_destructor& dtor) {
        REQUIRE(dtor.signature() == "()");
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
                                                                       cpp_token_string::tokenize(
                                                                           "false"))));
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
            if (dtor.is_declaration())
                REQUIRE(dtor.virtual_info().value()
                        == (cpp_virtual_flags::override | cpp_virtual_flags::final));
            else
                REQUIRE(dtor.virtual_info().value() == cpp_virtual_flags::override);
            REQUIRE(!dtor.noexcept_condition());
        }
        else if (dtor.name() == "~e")
        {
            REQUIRE(dtor.virtual_info());
            if (dtor.is_declaration())
                REQUIRE(dtor.virtual_info().value()
                        == (cpp_virtual_flags::override | cpp_virtual_flags::final));
            else
            {
                REQUIRE(dtor.virtual_info().value() == cpp_virtual_flags::override);
                REQUIRE(dtor.body_kind() == cpp_function_defaulted);
            }
            REQUIRE(!dtor.noexcept_condition());
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 7u);
}

TEST_CASE("consteval cpp_constructor")
{
    if (libclang_parser::libclang_minor_version() < 60)
        return;

    // only test constructor specific stuff
    const char* code;
    auto        is_template = false;
    SECTION("non-template")
    {
        code = R"(
struct foo
{
    /// consteval foo(int)=delete;
    consteval foo(int) = delete;
};

)";
    }
    SECTION("template")
    {
        is_template = true;
        code        = R"(
template <typename T>
struct foo
{
    /// consteval foo(int)=delete;
    consteval foo(int) = delete;
};

)";
    }
    INFO(is_template);

    cpp_entity_index idx;
    auto file  = parse(idx, "consteval_constructor.cpp", code, false, cppast::cpp_standard::cpp_2a);
    auto count = test_visit<cpp_constructor>(*file, [&](const cpp_constructor& cont) {
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

            if (count_children(cont.parameters()) == 1u)
            {
                REQUIRE(!cont.noexcept_condition());
                REQUIRE(!cont.is_explicit());
                REQUIRE(cont.is_consteval());
                REQUIRE(cont.body_kind() == cpp_function_deleted);
                REQUIRE(cont.signature() == "(int)");
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 1u);
}

TEST_CASE("consteval cpp_conversion_op")
{
    if (libclang_parser::libclang_minor_version() < 60)
        return;

    auto             code = R"(
namespace ns
{
    template <typename T>
    struct type2 {};
}

// most of it only need to be check in member function
struct foo
{
    /// consteval operator ns::type2<int>();
    consteval operator ns::type2<int>()
    {
        return {};
    }

    /// consteval operator ns::type2<char>();
    consteval operator ns::type2<char>()
    {
        return {};
    }
};
)";
    cpp_entity_index idx;
    auto file  = parse(idx, "consteval_op.cpp", code, false, cppast::cpp_standard::cpp_2a);
    auto count = test_visit<cpp_conversion_op>(*file, [&](const cpp_conversion_op& op) {
        REQUIRE(count_children(op.parameters()) == 0u);
        REQUIRE(!op.is_variadic());
        REQUIRE(op.body_kind() == cpp_function_definition);
        REQUIRE(op.ref_qualifier() == cpp_ref_none);
        REQUIRE(!op.virtual_info());
        REQUIRE(!op.noexcept_condition());

        if (!op.is_explicit() && op.is_consteval())
        {
            REQUIRE(op.cv_qualifier() == cpp_cv_none);
            REQUIRE(op.signature() == "()");
            if (op.name() == "operator ns::type2<int>")
            {
                REQUIRE(op.return_type().kind() == cpp_type_kind::template_instantiation_t);
                auto& inst = static_cast<const cpp_template_instantiation_type&>(op.return_type());

                REQUIRE(inst.primary_template().name() == "ns::type2");
                REQUIRE(!inst.arguments_exposed());
                REQUIRE(inst.unexposed_arguments() == "int");
            }
            else if (op.name() == "operator ns::type2<char>")
            {
                REQUIRE(op.return_type().kind() == cpp_type_kind::template_instantiation_t);
                auto& inst = static_cast<const cpp_template_instantiation_type&>(op.return_type());

                REQUIRE(inst.primary_template().name() == "ns::type2");
                REQUIRE(!inst.arguments_exposed());
                REQUIRE(inst.unexposed_arguments() == "char");
            }
            else
                REQUIRE(false);
        }
    });
    REQUIRE(count == 2u);
}

