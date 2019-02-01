// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_function_template.hpp>

#include <cppast/cpp_member_function.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_function_template")
{
    // only check templated related stuff
    auto code = R"(
template <int I>
using type = int;

/// template<typename T>
/// T a(T const& t);
template <typename T>
T a(const T& t);

struct d
{
    /// template<int I,typename T>
    /// static type<I> b(T);
    template <int I, typename T>
    static type<I> b(T);

    /// template<typename T=const int>
    /// T c();
    template <typename T = const int>
    auto c() -> T;

    /// template<typename T>
    /// operator T()const;
    template <typename T>
    operator T() const;

    /// template<typename T>
    /// d(T const&);
    template <typename T>
    d(const T&);
};

/// template<>
/// int a(int const& t);
template <>
int a(const int& t);

/// template<>
/// static type<0> d::b<0,int>(int);
template <>
type<0> d::b<0, int>(int);

/// template<>
/// int d::c();
template <>
auto d::c() -> int;

/// template<>
/// d::operator int()const;
template <>
d::operator int() const;

/// template<>
/// d::d(int const&);
template <>
d::d(const int&);
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_function_template.cpp", code);
    auto count = test_visit<cpp_function_template>(*file, [&](const cpp_function_template& tfunc) {
        REQUIRE(is_templated(tfunc.function()));
        REQUIRE(!tfunc.scope_name());

        if (tfunc.name() == "a")
        {
            check_template_parameters(tfunc, {{cpp_entity_kind::template_type_parameter_t, "T"}});

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::function_t);
            auto& func = static_cast<const cpp_function&>(tfunc.function());

            auto parameter = cpp_template_type_parameter_ref(cpp_entity_id(""), "T");
            REQUIRE(equal_types(idx, func.return_type(),
                                *cpp_template_parameter_type::build(parameter)));

            auto count = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(
                    equal_types(idx, param.type(),
                                *cpp_reference_type::
                                    build(cpp_cv_qualified_type::
                                              build(cpp_template_parameter_type::build(parameter),
                                                    cpp_cv_const),
                                          cpp_ref_lvalue)));
            }
            REQUIRE(count == 1u);
        }
        else if (tfunc.name() == "b")
        {
            check_parent(tfunc, "d", "d::b");
            check_template_parameters(tfunc, {{cpp_entity_kind::non_type_template_parameter_t, "I"},
                                              {cpp_entity_kind::template_type_parameter_t, "T"}});

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::function_t);
            auto& func = static_cast<const cpp_function&>(tfunc.function());

            cpp_template_instantiation_type::builder builder(
                cpp_template_ref(cpp_entity_id(""), "type"));
            builder.add_unexposed_arguments("I");
            REQUIRE(equal_types(idx, func.return_type(), *builder.finish()));

            auto type_parameter = cpp_template_type_parameter_ref(cpp_entity_id(""), "T");
            auto count          = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(equal_types(idx, param.type(),
                                    *cpp_template_parameter_type::build(type_parameter)));
            }
            REQUIRE(count == 1u);
        }
        else if (tfunc.name() == "c")
        {
            check_parent(tfunc, "d", "d::c");
            check_template_parameters(tfunc, {{cpp_entity_kind::template_type_parameter_t, "T"}});

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::member_function_t);
            auto& func = static_cast<const cpp_member_function&>(tfunc.function());
            REQUIRE(func.cv_qualifier() == cpp_cv_none);

            auto parameter = cpp_template_type_parameter_ref(cpp_entity_id(""), "T");
            REQUIRE(equal_types(idx, func.return_type(),
                                *cpp_template_parameter_type::build(parameter)));
        }
        else if (tfunc.name() == "operator T")
        {
            check_parent(tfunc, "d", "d::operator T");
            check_template_parameters(tfunc, {{cpp_entity_kind::template_type_parameter_t, "T"}});

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::conversion_op_t);
            auto& func = static_cast<const cpp_conversion_op&>(tfunc.function());
            REQUIRE(func.cv_qualifier() == cpp_cv_const);

            auto parameter = cpp_template_type_parameter_ref(cpp_entity_id(""), "T");
            REQUIRE(equal_types(idx, func.return_type(),
                                *cpp_template_parameter_type::build(parameter)));
        }
        else if (tfunc.name() == "d")
        {
            check_parent(tfunc, "d", "d::d");
            check_template_parameters(tfunc, {{cpp_entity_kind::template_type_parameter_t, "T"}});

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::constructor_t);
            auto& func = static_cast<const cpp_constructor&>(tfunc.function());

            auto parameter = cpp_template_type_parameter_ref(cpp_entity_id(""), "T");
            auto count     = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(
                    equal_types(idx, param.type(),
                                *cpp_reference_type::
                                    build(cpp_cv_qualified_type::
                                              build(cpp_template_parameter_type::build(parameter),
                                                    cpp_cv_const),
                                          cpp_ref_lvalue)));
            }
            REQUIRE(count == 1u);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);

    count = test_visit<
        cpp_function_template_specialization>(*file, [&](const cpp_function_template_specialization&
                                                             tfunc) {
        REQUIRE(tfunc.is_full_specialization());
        REQUIRE(!tfunc.arguments_exposed());
        REQUIRE(!tfunc.scope_name());

        auto templ = tfunc.primary_template();
        if (tfunc.name() == "operator int")
            REQUIRE(equal_ref(idx, templ, cpp_template_ref(cpp_entity_id(""), tfunc.name()),
                              "d::operator T"));
        else
            REQUIRE(equal_ref(idx, templ, cpp_template_ref(cpp_entity_id(""), tfunc.name()),
                              (tfunc.function().semantic_scope() + tfunc.name()).c_str()));

        if (tfunc.name() == "a")
        {
            REQUIRE(tfunc.unexposed_arguments().empty());

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::function_t);
            auto& func = static_cast<const cpp_function&>(tfunc.function());
            REQUIRE(func.semantic_scope() == "");

            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_int)));

            auto count = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(equal_types(idx, param.type(),
                                    *cpp_reference_type::
                                        build(cpp_cv_qualified_type::build(cpp_builtin_type::build(
                                                                               cpp_int),
                                                                           cpp_cv_const),
                                              cpp_ref_lvalue)));
            }
            REQUIRE(count == 1u);
        }
        else if (tfunc.name() == "b")
        {
            REQUIRE(tfunc.unexposed_arguments().as_string() == "0,int");

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::function_t);
            auto& func = static_cast<const cpp_function&>(tfunc.function());
            REQUIRE(func.semantic_scope() == "d::");

            cpp_template_instantiation_type::builder builder(
                cpp_template_ref(cpp_entity_id(""), "type"));
            builder.add_unexposed_arguments("0");
            REQUIRE(equal_types(idx, func.return_type(), *builder.finish()));

            auto count = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(equal_types(idx, param.type(), *cpp_builtin_type::build(cpp_int)));
            }
            REQUIRE(count == 1u);
        }
        else if (tfunc.name() == "c")
        {
            REQUIRE(tfunc.unexposed_arguments().empty());

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::member_function_t);
            auto& func = static_cast<const cpp_member_function&>(tfunc.function());
            REQUIRE(func.semantic_scope() == "d::");
            REQUIRE(func.cv_qualifier() == cpp_cv_none);

            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_int)));
        }
        else if (tfunc.name() == "operator int")
        {
            REQUIRE(tfunc.unexposed_arguments().empty());

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::conversion_op_t);
            auto& func = static_cast<const cpp_conversion_op&>(tfunc.function());
            REQUIRE(func.cv_qualifier() == cpp_cv_const);

            REQUIRE(equal_types(idx, func.return_type(), *cpp_builtin_type::build(cpp_int)));
        }
        else if (tfunc.name() == "d")
        {
            REQUIRE(tfunc.unexposed_arguments().empty());

            REQUIRE(tfunc.function().kind() == cpp_entity_kind::constructor_t);
            auto& func = static_cast<const cpp_constructor&>(tfunc.function());
            REQUIRE(func.semantic_scope() == "d::");

            auto count = 0u;
            for (auto& param : func.parameters())
            {
                ++count;
                REQUIRE(equal_types(idx, param.type(),
                                    *cpp_reference_type::
                                        build(cpp_cv_qualified_type::build(cpp_builtin_type::build(
                                                                               cpp_int),
                                                                           cpp_cv_const),
                                              cpp_ref_lvalue)));
            }
            REQUIRE(count == 1u);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);
}
