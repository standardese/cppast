// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_class_template.hpp>

#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_class_template")
{
    auto code = R"(
// check everything not related to members first
/// template<typename T>
/// class a{
/// };
template <typename T>
class a {};

/// template<int I,typename T>
/// struct b{
/// };
template <int I, typename T>
struct b {};

/// template<template<typename>class T>
/// union c;
template <template <typename> class T>
union c;

// bases
/// template<typename T>
/// struct d
/// :T,a<T>,T::type,a<T>::type{
/// };
template <typename T>
struct d : T, a<T>, T::type, a<T>::type {};

// members
/// template<typename T>
/// struct e{
///   T var_a;
///
///   a<T> var_b;
///
///   typename T::type var_c;
///
///   typename a<T>::type var_d;
///
///   template<typename U>
///   T func(U);
/// };
template <typename T>
struct e
{
    T var_a;
    a<T> var_b;
    typename T::type var_c;
    typename a<T>::type var_d;

    template <typename U>
    T func(U);
};

// full specialization
/// template<>
/// class a<int>{
/// };
template <>
class a<int> {};

/// template<>
/// struct b<0,int>{
/// };
template <>
struct b<0, int> {};

// partial specialization
/// template<typename T>
/// class a<T*>{
/// };
template <typename T>
class a<T*> {};

/// template<typename T>
/// struct b<0,T>{
/// };
template <typename T>
struct b<0, T> {};

// extern template for good measure
// currently not really supported but parsing must not fail
/// template<>
/// class a<int>;
extern template class a<int>;

// non-extern template as well
/// template<>
/// class a<int>;
template class a<int>;
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_class_template.cpp", code);
    auto count = test_visit<cpp_class_template>(*file, [&](const cpp_class_template& templ) {
        auto& c = templ.class_();
        REQUIRE(!c.is_final());
        REQUIRE(is_templated(c));
        REQUIRE(templ.scope_name());

        if (templ.name() == "a")
        {
            check_template_parameters(templ, {{cpp_entity_kind::template_type_parameter_t, "T"}});
            REQUIRE(c.class_kind() == cpp_class_kind::class_t);
            REQUIRE(c.is_definition());
        }
        else if (templ.name() == "b")
        {
            check_template_parameters(templ, {{cpp_entity_kind::non_type_template_parameter_t, "I"},
                                              {cpp_entity_kind::template_type_parameter_t, "T"}});
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(c.is_definition());
        }
        else if (templ.name() == "c")
        {
            check_template_parameters(templ,
                                      {{cpp_entity_kind::template_template_parameter_t, "T"}});
            REQUIRE(c.class_kind() == cpp_class_kind::union_t);
            REQUIRE(c.is_declaration());
        }
        else
        {
            check_template_parameters(templ, {{cpp_entity_kind::template_type_parameter_t, "T"}});
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(c.is_definition());

            if (templ.name() == "d")
            {
                auto no = 0u;
                for (auto& base : c.bases())
                {
                    ++no;

                    REQUIRE(base.access_specifier() == cpp_public);
                    REQUIRE(!base.is_virtual());

                    if (base.name() == "T")
                        REQUIRE(equal_types(idx, base.type(),
                                            *cpp_template_parameter_type::build(
                                                cpp_template_type_parameter_ref(cpp_entity_id(""),
                                                                                "T"))));
                    else if (base.name() == "a<T>")
                    {
                        cpp_template_instantiation_type::builder builder(
                            cpp_template_ref(cpp_entity_id(""), "a"));
                        builder.add_unexposed_arguments("T");
                        REQUIRE(equal_types(idx, base.type(), *builder.finish()));
                    }
                    else if (base.name() == "T::type")
                        REQUIRE(
                            equal_types(idx, base.type(), *cpp_unexposed_type::build("T::type")));
                    else if (base.name() == "a<T>::type")
                        REQUIRE(equal_types(idx, base.type(),
                                            *cpp_unexposed_type::build("a<T>::type")));
                    else
                        REQUIRE(false);
                }
                REQUIRE(no == 4u);
            }
            else if (templ.name() == "e")
            {
                auto no = 0u;
                for (auto& child : c)
                {
                    ++no;

                    if (child.name().length() == 5u && child.name().substr(0, 3u) == "var")
                    {
                        REQUIRE(child.kind() == cpp_entity_kind::member_variable_t);
                        auto& var = static_cast<const cpp_member_variable&>(child);
                        if (child.name() == "var_a")
                            REQUIRE(
                                equal_types(idx, var.type(),
                                            *cpp_template_parameter_type::build(
                                                cpp_template_type_parameter_ref(cpp_entity_id(""),
                                                                                "T"))));
                        else if (child.name() == "var_b")
                        {
                            cpp_template_instantiation_type::builder builder(
                                cpp_template_ref(cpp_entity_id(""), "a"));
                            builder.add_unexposed_arguments("T");
                            REQUIRE(equal_types(idx, var.type(), *builder.finish()));
                        }
                        else if (child.name() == "var_c")
                            REQUIRE(equal_types(idx, var.type(),
                                                *cpp_unexposed_type::build("typename T::type")));
                        else if (child.name() == "var_d")
                            REQUIRE(equal_types(idx, var.type(),
                                                *cpp_unexposed_type::build("typename a<T>::type")));
                        else
                            REQUIRE(false);
                    }
                    else if (child.name() == "func")
                    {
                        REQUIRE(child.kind() == cpp_entity_kind::function_template_t);
                        auto& templ = static_cast<const cpp_function_template&>(child);

                        REQUIRE(templ.function().kind() == cpp_entity_kind::member_function_t);
                        auto& mfunc = static_cast<const cpp_member_function&>(templ.function());

                        REQUIRE(equal_types(idx, mfunc.return_type(),
                                            *cpp_template_parameter_type::build(
                                                cpp_template_type_parameter_ref(cpp_entity_id(""),
                                                                                "T"))));
                        for (auto& param : mfunc.parameters())
                            REQUIRE(
                                equal_types(idx, param.type(),
                                            *cpp_template_parameter_type::build(
                                                cpp_template_type_parameter_ref(cpp_entity_id(""),
                                                                                "U"))));
                    }
                    else
                        REQUIRE(false);
                }
                REQUIRE(no == 5u);
            }
        }
    });
    REQUIRE(count == 5u);

    count = test_visit<
        cpp_class_template_specialization>(*file, [&](const cpp_class_template_specialization&
                                                          templ) {
        REQUIRE(!templ.arguments_exposed());
        REQUIRE(templ.scope_name());

        if (templ.name() == "a")
        {
            REQUIRE(
                equal_ref(idx, templ.primary_template(), cpp_template_ref(cpp_entity_id(""), "a")));
            if (templ.is_full_specialization())
            {
                check_template_parameters(templ, {});
                REQUIRE(templ.unexposed_arguments().as_string() == "int");
            }
            else
            {
                check_template_parameters(templ,
                                          {{cpp_entity_kind::template_type_parameter_t, "T"}});
                REQUIRE(templ.unexposed_arguments().as_string() == "T*");
            }
        }
        else if (templ.name() == "b")
        {
            REQUIRE(
                equal_ref(idx, templ.primary_template(), cpp_template_ref(cpp_entity_id(""), "b")));
            if (templ.is_full_specialization())
            {
                check_template_parameters(templ, {});
                REQUIRE(templ.unexposed_arguments().as_string() == "0,int");
            }
            else
            {
                check_template_parameters(templ,
                                          {{cpp_entity_kind::template_type_parameter_t, "T"}});
                REQUIRE(templ.unexposed_arguments().as_string() == "0,T");
            }
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 6u);
}
