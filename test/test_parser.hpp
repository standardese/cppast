// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_TEST_PARSER_HPP_INCLUDED
#define CPPAST_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include <catch.hpp>

#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>

inline void write_file(const char* name, const char* code)
{
    std::ofstream file(name);
    file << code;
}

inline std::unique_ptr<cppast::cpp_file> parse(const cppast::cpp_entity_index& idx,
                                               const char* name, const char* code)
{
    using namespace cppast;

    write_file(name, code);

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);

    static stderr_diagnostic_logger logger;
    libclang_parser                 p(type_safe::ref(logger));

    auto result = p.parse(idx, name, config);
    REQUIRE(!logger.error_logged());
    return result;
}

template <typename T, typename Func>
unsigned test_visit(const cppast::cpp_file& file, Func f)
{
    auto count = 0u;
    cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (info == cppast::visitor_info::container_entity_exit)
            return true; // already handled

        if (e.kind() == T::kind())
        {
            auto& obj = static_cast<const T&>(e);
            f(obj);
            ++count;
        }

        return true;
    });

    return count;
}

// number of direct children
template <class Entity>
unsigned count_children(const Entity& cont)
{
    return std::distance(cont.begin(), cont.end());
}

// checks the full name/parent
inline void check_parent(const cppast::cpp_entity& e, const char* parent_name,
                         const char* full_name)
{
    REQUIRE(e.parent());
    REQUIRE(e.parent().value().name() == parent_name);
    REQUIRE(cppast::full_name(e) == full_name);
}

bool equal_types(const cppast::cpp_entity_index& idx, const cppast::cpp_type& parsed,
                 const cppast::cpp_type& synthesized);

inline bool equal_expressions(const cppast::cpp_expression& parsed,
                              const cppast::cpp_expression& synthesized)
{
    using namespace cppast;

    if (parsed.kind() != synthesized.kind())
        return false;
    switch (parsed.kind())
    {
    case cpp_expression_kind::unexposed:
        return static_cast<const cpp_unexposed_expression&>(parsed).expression()
               == static_cast<const cpp_unexposed_expression&>(synthesized).expression();

    case cpp_expression_kind::literal:
        return static_cast<const cpp_literal_expression&>(parsed).value()
               == static_cast<const cpp_literal_expression&>(synthesized).value();
    }

    return false;
}

template <typename T>
void check_template_parameters(
    const T& templ, std::initializer_list<std::pair<cppast::cpp_entity_kind, const char*>> params)
{
    // no need to check more
    auto cur = params.begin();
    for (auto& param : templ.parameters())
    {
        REQUIRE(cur != params.end());
        REQUIRE(param.kind() == cur->first);
        REQUIRE(param.name() == cur->second);
        ++cur;
    }
    REQUIRE(cur == params.end());
}

#endif // CPPAST_TEST_PARSER_HPP_INCLUDED
