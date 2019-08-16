// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type_alias.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_decltype_type.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_template_parameter.hpp>

#include "test_parser.hpp"

using namespace cppast;

bool equal_types(const cpp_entity_index& idx, const cpp_type& parsed, const cpp_type& synthesized)
{
    if (parsed.kind() != synthesized.kind())
        return false;

    switch (parsed.kind())
    {
    case cpp_type_kind::builtin_t:
        return static_cast<const cpp_builtin_type&>(parsed).builtin_type_kind()
               == static_cast<const cpp_builtin_type&>(synthesized).builtin_type_kind();

    case cpp_type_kind::user_defined_t:
    {
        auto& user_parsed      = static_cast<const cpp_user_defined_type&>(parsed);
        auto& user_synthesized = static_cast<const cpp_user_defined_type&>(synthesized);
        return equal_ref(idx, user_parsed.entity(), user_synthesized.entity());
    }

    case cpp_type_kind::auto_t:
        return true;
    case cpp_type_kind::decltype_t:
        return equal_expressions(static_cast<const cpp_decltype_type&>(parsed).expression(),
                                 static_cast<const cpp_decltype_type&>(synthesized).expression());
    case cpp_type_kind::decltype_auto_t:
        return true;

    case cpp_type_kind::cv_qualified_t:
    {
        auto& cv_a = static_cast<const cpp_cv_qualified_type&>(parsed);
        auto& cv_b = static_cast<const cpp_cv_qualified_type&>(synthesized);
        return cv_a.cv_qualifier() == cv_b.cv_qualifier()
               && equal_types(idx, cv_a.type(), cv_b.type());
    }

    case cpp_type_kind::pointer_t:
        return equal_types(idx, static_cast<const cpp_pointer_type&>(parsed).pointee(),
                           static_cast<const cpp_pointer_type&>(synthesized).pointee());
    case cpp_type_kind::reference_t:
    {
        auto& ref_a = static_cast<const cpp_reference_type&>(parsed);
        auto& ref_b = static_cast<const cpp_reference_type&>(synthesized);
        return ref_a.reference_kind() == ref_b.reference_kind()
               && equal_types(idx, ref_a.referee(), ref_b.referee());
    }

    case cpp_type_kind::array_t:
    {
        auto& array_a = static_cast<const cpp_array_type&>(parsed);
        auto& array_b = static_cast<const cpp_array_type&>(synthesized);

        // check value type
        if (!equal_types(idx, array_a.value_type(), array_b.value_type()))
            return false;

        // check size
        if (!array_a.size().has_value() && !array_b.size().has_value())
            return true;

        auto& size_a = array_a.size().value();
        auto& size_b = array_b.size().value();
        return equal_expressions(size_a, size_b);
    }

    case cpp_type_kind::function_t:
    {
        auto& func_a = static_cast<const cpp_function_type&>(parsed);
        auto& func_b = static_cast<const cpp_function_type&>(synthesized);

        if (!equal_types(idx, func_a.return_type(), func_b.return_type()))
            return false;
        else if (func_a.is_variadic() != func_b.is_variadic())
            return false;

        auto iter_a = func_a.parameter_types().begin();
        auto iter_b = func_b.parameter_types().begin();
        while (iter_a != func_a.parameter_types().end() && iter_b != func_b.parameter_types().end())
        {
            if (!equal_types(idx, *iter_a, *iter_b))
                return false;
            ++iter_a;
            ++iter_b;
        }
        return iter_a == func_a.parameter_types().end() && iter_b == func_b.parameter_types().end();
    }
    case cpp_type_kind::member_function_t:
    {
        auto& func_a = static_cast<const cpp_member_function_type&>(parsed);
        auto& func_b = static_cast<const cpp_member_function_type&>(synthesized);

        if (!equal_types(idx, func_a.return_type(), func_b.return_type()))
            return false;
        else if (!equal_types(idx, func_a.class_type(), func_b.class_type()))
            return false;
        else if (func_a.is_variadic() != func_b.is_variadic())
            return false;

        auto iter_a = func_a.parameter_types().begin();
        auto iter_b = func_b.parameter_types().begin();
        while (iter_a != func_a.parameter_types().end() && iter_b != func_b.parameter_types().end())
        {
            if (!equal_types(idx, *iter_a, *iter_b))
                return false;
            ++iter_a;
            ++iter_b;
        }
        return iter_a == func_a.parameter_types().end() && iter_b == func_b.parameter_types().end();
    }
    case cpp_type_kind::member_object_t:
    {
        auto& obj_a = static_cast<const cpp_member_object_type&>(parsed);
        auto& obj_b = static_cast<const cpp_member_object_type&>(synthesized);

        if (!equal_types(idx, obj_a.class_type(), obj_b.class_type()))
            return false;
        return equal_types(idx, obj_a.object_type(), obj_b.object_type());
    }

    case cpp_type_kind::template_parameter_t:
    {
        auto& entity_parsed = static_cast<const cpp_template_parameter_type&>(parsed).entity();
        auto& entity_synthesized
            = static_cast<const cpp_template_parameter_type&>(synthesized).entity();
        return equal_ref(idx, entity_parsed, entity_synthesized);
    }
    case cpp_type_kind::template_instantiation_t:
    {
        auto& inst_parsed      = static_cast<const cpp_template_instantiation_type&>(parsed);
        auto& inst_synthesized = static_cast<const cpp_template_instantiation_type&>(synthesized);

        if (!equal_ref(idx, inst_parsed.primary_template(), inst_synthesized.primary_template()))
            return false;
        else if (inst_parsed.arguments_exposed() != inst_synthesized.arguments_exposed())
            return false;
        else if (!inst_parsed.arguments_exposed())
            return inst_parsed.unexposed_arguments() == inst_synthesized.unexposed_arguments();
        else if (inst_parsed.arguments().has_value() != inst_synthesized.arguments().has_value())
            return false;
        else if (!inst_parsed.arguments().has_value())
            return true;

        auto iter_a = inst_parsed.arguments().value().begin();
        auto iter_b = inst_synthesized.arguments().value().begin();
        while (iter_a != inst_parsed.arguments().value().end()
               && iter_b != inst_synthesized.arguments().value().end())
        {
            if (iter_a->type().has_value() && iter_b->type().has_value())
            {
                if (!equal_types(idx, iter_a->type().value(), iter_b->type().value()))
                    return false;
            }
            else if (iter_a->expression().has_value() && iter_b->expression().has_value())
            {
                if (!equal_expressions(iter_a->expression().value(), iter_b->expression().value()))
                    return false;
            }
            else if (iter_a->template_ref().has_value() && iter_b->template_ref().has_value())
            {
                if (!equal_ref(idx, iter_a->template_ref().value(), iter_b->template_ref().value()))
                    return false;
            }
            else
                return false;
            ++iter_a;
            ++iter_b;
        }
        return iter_a == inst_parsed.arguments().value().end()
               && iter_b == inst_synthesized.arguments().value().end();
    }
    case cpp_type_kind::dependent_t:
    {
        auto& dep_a = static_cast<const cpp_dependent_type&>(parsed);
        auto& dep_b = static_cast<const cpp_dependent_type&>(synthesized);
        return dep_a.name() == dep_b.name() && equal_types(idx, dep_a.dependee(), dep_b.dependee());
    }

    case cpp_type_kind::unexposed_t:
        return static_cast<const cpp_unexposed_type&>(parsed).name()
               == static_cast<const cpp_unexposed_type&>(synthesized).name();
    }

    return false;
}

// also tests the type parsing code
// other test cases don't need that anymore
TEST_CASE("cpp_type_alias")
{
    const char* code       = nullptr;
    auto        check_code = false;
    SECTION("using")
    {
        check_code = true;
        code       = R"(
// basic
/// using a=int;
using a = int;
/// using b=long double const volatile;
using b = const long double volatile;

// pointers
/// using c=int*;
using c = int*;
/// using d=unsigned int const*;
using d = const unsigned int*;
/// using e=unsigned int const* volatile;
using e = unsigned const * volatile;

// references
/// using f=int&;
using f = int&;
/// using g=int const&&;
using g = const int&&;

// user-defined types
/// using h=c;
using h = c;
/// using i=d const;
using i = const d;
/// using j=e*;
using j = e*;

// arrays
/// using k=int[42];
using k = int[42];
/// using l=float*[];
using l = float*[];
/// using m=char[42];
using m = char[3 * 2 + 4 ? 42 : 43];

// function pointers
/// using n=void(*)(int);
using n = void(*)(int);
/// using o=char*(&)(int const&,...);
using o = char*(&)(const int&,...);
/// using p=n(*)(int,o);
using p = n(*)(int, o);

struct foo {};

// member function pointers
/// using q=void(foo::*)(int);
using q = void(foo::*)(int);
/// using r=void(foo::*)(int,...)const&;
using r = void(foo::*)(int,...) const &;

// member data pointers
/// using s=int(foo::*);
using s = int(foo::*);

// user-defined types inline definition
using t = struct t_ {};
using u = const struct u_ {}*;
using v = struct {};

// decltype
/// using w=decltype(0);
using w = decltype(0);
)";
    }
    SECTION("typedef")
    {
        check_code = false; // will always generate using
        code       = R"(
// basic
typedef int a;
typedef const long double volatile b;

// pointers
typedef int* c;
typedef const unsigned int* d;
typedef unsigned const * volatile e;

// references
typedef int& f;
typedef const int&& g;

// user-defined types
typedef c h;
typedef const d i;
typedef e* j;

// arrays
typedef int k[42];
typedef float* l[];
typedef char m[3 * 2 + 4 ? 42 : 43];

// function pointers
typedef void(*n)(int);
typedef char*(&o)(const int&,...);
typedef n(*p)(int, o);

struct foo {};

// member function pointers
typedef void(foo::*q)(int);
typedef void(foo::*r)(int,...) const &;

// member data pointers
typedef int(foo::*s);

// user-defined types inline definition
typedef struct t_ {} t;
typedef const struct u_ {}* u;
typedef struct {} v;

// decltype
typedef decltype(0) w;
)";
    }

    auto add_cv = [](std::unique_ptr<cpp_type> type, cpp_cv cv) {
        return cpp_cv_qualified_type::build(std::move(type), cv);
    };

    auto make_size = [](std::string size, bool literal) -> std::unique_ptr<cpp_expression> {
        auto type = cpp_builtin_type::build(cpp_ulonglong);
        if (literal)
            return cpp_literal_expression::build(std::move(type), std::move(size));
        else
            return cpp_unexposed_expression::build(std::move(type),
                                                   cpp_token_string::tokenize(std::move(size)));
    };

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_type_alias.cpp", code);
    auto             count = test_visit<cpp_type_alias>(*file, [&](const cpp_type_alias& alias) {
        if (alias.name() == "a")
        {
            auto type = cpp_builtin_type::build(cpp_int);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "b")
        {
            auto type = add_cv(cpp_builtin_type::build(cpp_longdouble), cpp_cv_const_volatile);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "c")
        {
            auto type = cpp_pointer_type::build(cpp_builtin_type::build(cpp_int));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "d")
        {
            auto type
                = cpp_pointer_type::build(add_cv(cpp_builtin_type::build(cpp_uint), cpp_cv_const));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "e")
        {
            auto type = add_cv(cpp_pointer_type::build(
                                   add_cv(cpp_builtin_type::build(cpp_uint), cpp_cv_const)),
                               cpp_cv_volatile);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "f")
        {
            auto type = cpp_reference_type::build(cpp_builtin_type::build(cpp_int), cpp_ref_lvalue);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "g")
        {
            auto type
                = cpp_reference_type::build(add_cv(cpp_builtin_type::build(cpp_int), cpp_cv_const),
                                            cpp_ref_rvalue);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "h")
        {
            auto type = cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "c"));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "i")
        {
            auto type = add_cv(cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "d")),
                               cpp_cv_const);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "j")
        {
            auto type = cpp_pointer_type::build(
                cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "e")));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "k")
        {
            auto type
                = cpp_array_type::build(cpp_builtin_type::build(cpp_int), make_size("42", true));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "l")
        {
            auto type
                = cpp_array_type::build(cpp_pointer_type::build(cpp_builtin_type::build(cpp_float)),
                                        nullptr);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "m")
        {
            auto type
                = cpp_array_type::build(cpp_builtin_type::build(cpp_char), make_size("42", true));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "n")
        {
            cpp_function_type::builder builder(cpp_builtin_type::build(cpp_void));
            builder.add_parameter(cpp_builtin_type::build(cpp_int));
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "o")
        {
            cpp_function_type::builder builder(
                cpp_pointer_type::build(cpp_builtin_type::build(cpp_char)));
            builder.add_parameter(
                cpp_reference_type::build(add_cv(cpp_builtin_type::build(cpp_int), cpp_cv_const),
                                          cpp_ref_lvalue));
            builder.is_variadic();
            auto type = cpp_reference_type::build(builder.finish(), cpp_ref_lvalue);

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "p")
        {
            cpp_function_type::builder builder(
                cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "n")));
            builder.add_parameter(cpp_builtin_type::build(cpp_int));
            builder.add_parameter(
                cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "o")));
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "q")
        {
            cpp_member_function_type::builder builder(cpp_user_defined_type::build(
                                                          cpp_type_ref(cpp_entity_id(""), "foo")),
                                                      cpp_builtin_type::build(cpp_void));
            builder.add_parameter(cpp_builtin_type::build(cpp_int));
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "r")
        {
            auto obj = cpp_reference_type::build(add_cv(cpp_user_defined_type::build(
                                                            cpp_type_ref(cpp_entity_id(""), "foo")),
                                                        cpp_cv_const),
                                                 cpp_ref_lvalue);

            cpp_member_function_type::builder builder(std::move(obj),
                                                      cpp_builtin_type::build(cpp_void));
            builder.add_parameter(cpp_builtin_type::build(cpp_int));
            builder.is_variadic();
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "s")
        {
            // in old libclang version, the member type isn't exposed for some reason
            auto pointee
                = cpp_member_object_type::build(cpp_user_defined_type::build(
                                                    cpp_type_ref(cpp_entity_id(""), "foo")),
                                                cpp_unexposed_type::build("int"));
            auto type         = cpp_pointer_type::build(std::move(pointee));
            auto is_unexposed = equal_types(idx, alias.underlying_type(), *type);

            if (!is_unexposed)
            {
                pointee = cpp_member_object_type::build(cpp_user_defined_type::build(
                                                            cpp_type_ref(cpp_entity_id(""), "foo")),
                                                        cpp_builtin_type::build(cpp_int));
                type    = cpp_pointer_type::build(std::move(pointee));
                auto is_builtin = equal_types(idx, alias.underlying_type(), *type);
                REQUIRE(is_builtin);
            }
        }
        else if (alias.name() == "t")
        {
            auto type = cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "t_"));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
            return false; // inhibit comment check for next three entities
            // as they can't be documented (will always apply to the inline type)
        }
        else if (alias.name() == "u")
        {
            auto type = cpp_pointer_type::build(
                add_cv(cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "u_")),
                       cpp_cv_const));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
            return false;
        }
        else if (alias.name() == "v")
        {
            auto type = cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), ""));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
            return false;
        }
        else if (alias.name() == "w")
        {
            auto type = cpp_decltype_type::build(
                cpp_unexposed_expression::build(cpp_builtin_type::build(cpp_int),
                                                cpp_token_string::tokenize("0")));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else
            REQUIRE(false);

        return check_code;
    });
    REQUIRE(count == 23u);
}
