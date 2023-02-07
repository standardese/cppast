// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_concept.hpp>

#include <cppast/cpp_function_template.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_concept")
{
    if (libclang_parser::libclang_minor_version() < 60)
        return;

    auto             code = R"(
#include <concepts>

/// template<typename T>
/// concept a = requires(T t, int i)
/// {
///   {t.a()};
///   {t.b()} -> std::copy_constructible;
///   {t.c(i)} -> std::same_as<int>;
///   typename T::inner;
/// };
template<typename T>
concept a = requires(T t, int i)
{
    {t.a()};
    {t.b()} -> std::copy_constructible;
    {t.c(i)} -> std::same_as<int>;
    typename T::inner;
};

/// template<typename T>
/// concept b = a<T> && std::constructible_from<T, int>;
template<typename T>
concept b = a<T> && std::constructible_from<T, int>;

/// template<typename T>
/// void f1(T param);
template<typename T>
    requires a<T>
void f1(T param);

/// template<b T>
/// void f2(T param);
template<b T>
void f2(T param);

/// template<std::convertible_to<int> T>
/// void f3(T param);
template<std::convertible_to<int> T>
void f3(T param);

)";
    cpp_entity_index idx;
    auto file = parse(idx, "cpp_concept.cpp", code, false, cppast::cpp_standard::cpp_20);

    auto count = test_visit<cpp_concept>(
        *file, [&](const cpp_concept& con) {}, false);
    REQUIRE(count == 2u);

    count = test_visit<cpp_function_template>(*file, [&](const cpp_function_template& tfunc) {
        REQUIRE(is_templated(tfunc.function()));
        REQUIRE(!tfunc.scope_name());
        check_template_parameters(tfunc, {{cpp_entity_kind::template_type_parameter_t, "T"}});

        if (tfunc.name() == "f1")
        {
            REQUIRE(static_cast<const cpp_template_type_parameter&>(*tfunc.parameters().begin())
                        .keyword()
                    == cpp_template_keyword::keyword_typename);
        }
        else if (tfunc.name() == "f2" || tfunc.name() == "f3")
        {
            REQUIRE(static_cast<const cpp_template_type_parameter&>(*tfunc.parameters().begin())
                        .keyword()
                    == cpp_template_keyword::concept_contraint);
        }
    });

    REQUIRE(count == 3u);
}
