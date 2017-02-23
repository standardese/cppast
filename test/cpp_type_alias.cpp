// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type_alias.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_function_type.hpp>

#include "test_parser.hpp"

using namespace cppast;

bool equal_types(const cpp_entity_index& idx, const cpp_type& parsed, const cpp_type& synthesized)
{
    if (parsed.kind() != synthesized.kind())
        return false;

    switch (parsed.kind())
    {
    case cpp_type_kind::builtin:
        return static_cast<const cpp_builtin_type&>(parsed).name()
               == static_cast<const cpp_builtin_type&>(synthesized).name();

    case cpp_type_kind::user_defined:
    {
        auto user_parsed      = static_cast<const cpp_user_defined_type&>(parsed).entity();
        auto user_synthesized = static_cast<const cpp_user_defined_type&>(synthesized).entity();
        if (user_parsed.name() != user_synthesized.name())
            return false;
        // check that the referring also works
        auto entity = user_parsed.get(idx);
        return entity.has_value() && entity.value().name() == user_parsed.name();
    }

    case cpp_type_kind::cv_qualified:
    {
        auto& cv_a = static_cast<const cpp_cv_qualified_type&>(parsed);
        auto& cv_b = static_cast<const cpp_cv_qualified_type&>(synthesized);
        return cv_a.cv_qualifier() == cv_b.cv_qualifier()
               && equal_types(idx, cv_a.type(), cv_b.type());
    }

    case cpp_type_kind::pointer:
        return equal_types(idx, static_cast<const cpp_pointer_type&>(parsed).pointee(),
                           static_cast<const cpp_pointer_type&>(synthesized).pointee());
    case cpp_type_kind::reference:
    {
        auto& ref_a = static_cast<const cpp_reference_type&>(parsed);
        auto& ref_b = static_cast<const cpp_reference_type&>(synthesized);
        return ref_a.reference_kind() == ref_b.reference_kind()
               && equal_types(idx, ref_a.referee(), ref_b.referee());
    }

    case cpp_type_kind::array:
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
        if (size_a.kind() != size_b.kind())
            return false;
        else if (size_a.kind() == cpp_expression_kind::literal)
            return static_cast<const cpp_literal_expression&>(size_a).value()
                   == static_cast<const cpp_literal_expression&>(size_b).value();
        else if (size_a.kind() == cpp_expression_kind::unexposed)
            return static_cast<const cpp_unexposed_expression&>(size_a).expression()
                   == static_cast<const cpp_unexposed_expression&>(size_b).expression();
        else
            break;
    }

    case cpp_type_kind::function:
    {
        auto& func_a = static_cast<const cpp_function_type&>(parsed);
        auto& func_b = static_cast<const cpp_function_type&>(synthesized);

        if (!equal_types(idx, func_a.return_type(), func_b.return_type()))
            return false;
        else if (func_a.is_variadic() != func_b.is_variadic())
            return false;

        auto iter_a = func_a.parameter_types().begin();
        auto iter_b = func_a.parameter_types().begin();
        while (iter_a != func_a.parameter_types().end() && iter_b != func_b.parameter_types().end())
        {
            if (!equal_types(idx, *iter_a, *iter_b))
                return false;
            ++iter_a;
            ++iter_b;
        }
        return iter_a == func_a.parameter_types().end() && iter_b == func_b.parameter_types().end();
    }

    // TODO
    case cpp_type_kind::member_function:
        break;
    case cpp_type_kind::member_object:
        break;
    case cpp_type_kind::template_parameter:
        break;
    case cpp_type_kind::template_instantiation:
        break;
    case cpp_type_kind::dependent:
        break;

    case cpp_type_kind::unexposed:
        return static_cast<const cpp_unexposed_type&>(parsed).name()
               == static_cast<const cpp_unexposed_type&>(synthesized).name();
    }

    return false;
}

// also tests the type parsing code
// other test cases don't need that anymore
TEST_CASE("cpp_type_alias")
{
    auto code = R"(
// basic
using a = int;
using b = const long double volatile;

// pointers
using c = int*;
using d = const unsigned int*;
using e = unsigned const * volatile;

// references
using f = int&;
using g = const int&&;

// user-defined types
using h = c;
using i = const d;
using j = e*;

// arrays
using k = int[42];
using l = float*[];
using m = char[3 * 2 + 4 ? 42 : 43];

// function pointers
using n = void(*)(int);
using o = char*(&)(int&,...);
using p = n(*)(int, o);
)";

    auto add_cv = [](std::unique_ptr<cpp_type> type, cpp_cv cv) {
        return cpp_cv_qualified_type::build(std::move(type), cv);
    };

    auto make_size = [](std::string size, bool literal) -> std::unique_ptr<cpp_expression> {
        auto type = cpp_builtin_type::build("unsigned long long");
        if (literal)
            return cpp_literal_expression::build(std::move(type), std::move(size));
        else
            return cpp_unexposed_expression::build(std::move(type), std::move(size));
    };

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_type_alias.cpp", code);
    auto count            = test_visit<cpp_type_alias>(*file, [&](const cpp_type_alias& alias) {
        if (alias.name() == "a")
        {
            auto type = cpp_builtin_type::build("int");
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "b")
        {
            auto type = add_cv(cpp_builtin_type::build("long double"), cpp_cv_const_volatile);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "c")
        {
            auto type = cpp_pointer_type::build(cpp_builtin_type::build("int"));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "d")
        {
            auto type = cpp_pointer_type::build(
                add_cv(cpp_builtin_type::build("unsigned int"), cpp_cv_const));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "e")
        {
            auto type = add_cv(cpp_pointer_type::build(
                                   add_cv(cpp_builtin_type::build("unsigned int"), cpp_cv_const)),
                               cpp_cv_volatile);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "f")
        {
            auto type = cpp_reference_type::build(cpp_builtin_type::build("int"), cpp_ref_lvalue);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "g")
        {
            auto type =
                cpp_reference_type::build(add_cv(cpp_builtin_type::build("int"), cpp_cv_const),
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
            auto type =
                cpp_array_type::build(cpp_builtin_type::build("int"), make_size("42", true));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "l")
        {
            auto type =
                cpp_array_type::build(cpp_pointer_type::build(cpp_builtin_type::build("float")),
                                      nullptr);
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "m")
        {
            auto type =
                cpp_array_type::build(cpp_builtin_type::build("char"), make_size("42", true));
            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "n")
        {
            cpp_function_type::builder builder(cpp_builtin_type::build("void"));
            builder.add_parameter(cpp_builtin_type::build("int"));
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "o")
        {
            cpp_function_type::builder builder(
                cpp_pointer_type::build(cpp_builtin_type::build("char")));
            builder.add_parameter(
                cpp_reference_type::build(cpp_builtin_type::build("int"), cpp_ref_lvalue));
            builder.is_variadic();
            auto type = cpp_reference_type::build(builder.finish(), cpp_ref_lvalue);

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else if (alias.name() == "p")
        {
            cpp_function_type::builder builder(
                cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "n")));
            builder.add_parameter(cpp_builtin_type::build("int"));
            builder.add_parameter(
                cpp_user_defined_type::build(cpp_type_ref(cpp_entity_id(""), "o")));
            auto type = cpp_pointer_type::build(builder.finish());

            REQUIRE(equal_types(idx, alias.underlying_type(), *type));
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 16u);
}
