// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_template_parameter.hpp>

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_function_type.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_template_type_parameter")
{
    auto code = R"(

template <typename A>
using a = void;

template <class ... B>
using b = void;

template <typename = const int*>
using c = void;

// libclang workaround when decltype here
template <class D = decltype(1 + 3)>
using d = void;

// potential maximal munch here, but ignore it
template <typename E = a<void> >
using e = void;
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_template_type_parameter.cpp", code);
    auto             count = test_visit<
        cpp_alias_template>(*file,
                            [&](const cpp_alias_template& alias) {
                                REQUIRE(equal_types(idx, alias.type_alias().underlying_type(),
                                                    *cpp_builtin_type::build(cpp_void)));

                                for (auto& p : alias.parameters())
                                {
                                    REQUIRE(p.kind() == cpp_entity_kind::template_type_parameter_t);

                                    auto& param
                                        = static_cast<const cpp_template_type_parameter&>(p);
                                    if (param.name() == "A")
                                    {
                                        REQUIRE(alias.name() == "a");
                                        REQUIRE(param.keyword()
                                                == cpp_template_keyword::keyword_typename);
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(!param.default_type());
                                    }
                                    else if (param.name() == "B")
                                    {
                                        REQUIRE(alias.name() == "b");
                                        REQUIRE(param.keyword()
                                                == cpp_template_keyword::keyword_class);
                                        REQUIRE(param.is_variadic());
                                        REQUIRE(!param.default_type());
                                    }
                                    else if (param.name() == "")
                                    {
                                        REQUIRE(alias.name() == "c");
                                        REQUIRE(param.keyword()
                                                == cpp_template_keyword::keyword_typename);
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_type().has_value());
                                        REQUIRE(
                                            equal_types(idx, param.default_type().value(),
                                                        *cpp_unexposed_type::build("const int*")));
                                    }
                                    else if (param.name() == "D")
                                    {
                                        REQUIRE(alias.name() == "d");
                                        REQUIRE(param.keyword()
                                                == cpp_template_keyword::keyword_class);
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_type().has_value());
                                        REQUIRE(equal_types(idx, param.default_type().value(),
                                                            *cpp_unexposed_type::build(
                                                                "decltype(1+3)")));
                                    }
                                    else if (param.name() == "E")
                                    {
                                        REQUIRE(alias.name() == "e");
                                        REQUIRE(param.keyword()
                                                == cpp_template_keyword::keyword_typename);
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_type().has_value());
                                        REQUIRE(equal_types(idx, param.default_type().value(),
                                                            *cpp_unexposed_type::build("a<void>")));
                                    }
                                    else
                                        REQUIRE(false);
                                }
                            },
                            false); // can't check synopsis with comments
    REQUIRE(count == 5u);
}

TEST_CASE("cpp_non_type_template_parameter")
{
    auto code = R"(
template <int A>
using a = void;

template <char* = nullptr>
using b = void;

template <int ... C>
using c = void;

template <void(* D)(...)>
using d = void;
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_non_type_template_parameter.cpp", code);
    auto             count = test_visit<
        cpp_alias_template>(*file,
                            [&](const cpp_alias_template& alias) {
                                REQUIRE(equal_types(idx, alias.type_alias().underlying_type(),
                                                    *cpp_builtin_type::build(cpp_void)));

                                for (auto& p : alias.parameters())
                                {
                                    REQUIRE(p.kind()
                                            == cpp_entity_kind::non_type_template_parameter_t);

                                    auto& param
                                        = static_cast<const cpp_non_type_template_parameter&>(p);
                                    if (param.name() == "A")
                                    {
                                        REQUIRE(alias.name() == "a");
                                        REQUIRE(equal_types(idx, param.type(),
                                                            *cpp_builtin_type::build(cpp_int)));
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(!param.default_value());
                                    }
                                    else if (param.name() == "")
                                    {
                                        REQUIRE(alias.name() == "b");
                                        REQUIRE(
                                            equal_types(idx, param.type(),
                                                        *cpp_pointer_type::build(
                                                            cpp_builtin_type::build(cpp_char))));
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_value());
                                        REQUIRE(
                                            equal_expressions(param.default_value().value(),
                                                              *cpp_unexposed_expression::
                                                                  build(cpp_builtin_type::build(
                                                                            cpp_nullptr),
                                                                        cpp_token_string::tokenize(
                                                                            "nullptr"))));
                                    }
                                    else if (param.name() == "C")
                                    {
                                        REQUIRE(alias.name() == "c");
                                        REQUIRE(equal_types(idx, param.type(),
                                                            *cpp_builtin_type::build(cpp_int)));
                                        REQUIRE(param.is_variadic());
                                        REQUIRE(!param.default_value());
                                    }
                                    else if (param.name() == "D")
                                    {
                                        REQUIRE(alias.name() == "d");

                                        cpp_function_type::builder builder(
                                            cpp_builtin_type::build(cpp_void));
                                        builder.is_variadic();
                                        REQUIRE(equal_types(idx, param.type(),
                                                            *cpp_pointer_type::build(
                                                                builder.finish())));

                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(!param.default_value());
                                    }
                                    else
                                        REQUIRE(false);
                                }
                            },
                            false); // can't check synopsis with comments
    REQUIRE(count == 4u);
}

TEST_CASE("cpp_template_template_parameter")
{
    // no need to check parameters of template parameter
    auto code = R"(
namespace ns
{
    template <int I>
    using def = void;
}

template <template <typename T> class A>
using a = void;

template <template <int, typename> class ... B>
using b = void;

template <template <int> class C = ns::def>
using c = void;

template <template <template <typename...> class> class D = a>
using d = void;
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_template_template_parameter.cpp", code);
    auto             count = test_visit<
        cpp_alias_template>(*file,
                            [&](const cpp_alias_template& alias) {
                                REQUIRE(equal_types(idx, alias.type_alias().underlying_type(),
                                                    *cpp_builtin_type::build(cpp_void)));
                                if (alias.name() == "def")
                                    return;

                                for (auto& p : alias.parameters())
                                {
                                    REQUIRE(p.kind()
                                            == cpp_entity_kind::template_template_parameter_t);

                                    auto& param
                                        = static_cast<const cpp_template_template_parameter&>(p);
                                    REQUIRE(param.keyword() == cpp_template_keyword::keyword_class);
                                    if (param.name() == "A")
                                    {
                                        REQUIRE(alias.name() == "a");
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(!param.default_template());

                                        auto no = 0u;
                                        for (auto& p_param : param.parameters())
                                        {
                                            ++no;
                                            REQUIRE(p_param.name() == "T");
                                            REQUIRE(p_param.kind()
                                                    == cpp_entity_kind::template_type_parameter_t);
                                        }
                                        REQUIRE(no == 1u);
                                    }
                                    else if (param.name() == "B")
                                    {
                                        REQUIRE(alias.name() == "b");
                                        REQUIRE(param.is_variadic());
                                        REQUIRE(!param.default_template());

                                        auto cur = param.parameters().begin();
                                        REQUIRE(cur != param.parameters().end());
                                        REQUIRE(cur->name().empty());
                                        REQUIRE(cur->kind()
                                                == cpp_entity_kind::non_type_template_parameter_t);

                                        ++cur;
                                        REQUIRE(cur != param.parameters().end());
                                        REQUIRE(cur->name().empty());
                                        REQUIRE(cur->kind()
                                                == cpp_entity_kind::template_type_parameter_t);

                                        ++cur;
                                        REQUIRE(cur == param.parameters().end());
                                    }
                                    else if (param.name() == "C")
                                    {
                                        REQUIRE(alias.name() == "c");
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_template());

                                        auto def = param.default_template().value();
                                        REQUIRE(def.name() == "ns::def");
                                        auto entities = def.get(idx);
                                        REQUIRE(entities.size() == 1u);
                                        REQUIRE(entities[0]->name() == "def");

                                        auto no = 0u;
                                        for (auto& p_param : param.parameters())
                                        {
                                            ++no;
                                            REQUIRE(p_param.name() == "");
                                            REQUIRE(
                                                p_param.kind()
                                                == cpp_entity_kind::non_type_template_parameter_t);
                                        }
                                        REQUIRE(no == 1u);
                                    }
                                    else if (param.name() == "D")
                                    {
                                        REQUIRE(alias.name() == "d");
                                        REQUIRE(!param.is_variadic());
                                        REQUIRE(param.default_template());

                                        auto def = param.default_template().value();
                                        REQUIRE(def.name() == "a");
                                        auto entities = def.get(idx);
                                        REQUIRE(entities.size() == 1u);
                                        REQUIRE(entities[0]->name() == "a");

                                        auto no = 0u;
                                        for (auto& p_param : param.parameters())
                                        {
                                            ++no;
                                            REQUIRE(p_param.name() == "");
                                            REQUIRE(
                                                p_param.kind()
                                                == cpp_entity_kind::template_template_parameter_t);
                                            for (auto& p_p_param :
                                                 static_cast<
                                                     const cpp_template_template_parameter&>(
                                                     p_param)
                                                     .parameters())
                                            {
                                                ++no;
                                                REQUIRE(p_p_param.name() == "");
                                                REQUIRE(
                                                    p_p_param.kind()
                                                    == cpp_entity_kind::template_type_parameter_t);
                                                REQUIRE(p_p_param.is_variadic());
                                            }
                                        }
                                        REQUIRE(no == 2u);
                                    }
                                    else
                                        REQUIRE(false);
                                }
                            },
                            false); // can't check synopsis with comments
    REQUIRE(count == 5u);
}
