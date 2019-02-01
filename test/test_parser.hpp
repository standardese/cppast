// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_TEST_PARSER_HPP_INCLUDED
#define CPPAST_TEST_PARSER_HPP_INCLUDED

#include <fstream>

#include <catch.hpp>

#include <cppast/code_generator.hpp>
#include <cppast/cpp_class.hpp>
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

inline cppast::libclang_compile_config make_test_config()
{
    using namespace cppast;

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);
    return config;
}

inline std::unique_ptr<cppast::cpp_file> parse_file(const cppast::cpp_entity_index& idx,
                                                    const char*                     name,
                                                    bool fast_preprocessing = false)
{
    using namespace cppast;
    static auto config = make_test_config();
    config.fast_preprocessing(fast_preprocessing);

    libclang_parser p(default_logger());

    std::unique_ptr<cppast::cpp_file> result;
    REQUIRE_NOTHROW(result = p.parse(idx, name, config));
    REQUIRE(!p.error());
    return result;
}

inline std::unique_ptr<cppast::cpp_file> parse(const cppast::cpp_entity_index& idx,
                                               const char* name, const char* code,
                                               bool fast_preprocessing = false)
{
    write_file(name, code);
    return parse_file(idx, name, fast_preprocessing);
}

class test_generator : public cppast::code_generator
{
public:
    test_generator(generation_options options) : options_(std::move(options)) {}

    const std::string& str() const noexcept
    {
        return str_;
    }

private:
    generation_options do_get_options(const cppast::cpp_entity&,
                                      cppast::cpp_access_specifier_kind) override
    {
        return options_;
    }

    void do_indent() override
    {
        ++indent_;
    }

    void do_unindent() override
    {
        if (indent_)
            --indent_;
    }

    void do_write_token_seq(cppast::string_view tokens) override
    {
        if (was_newline_)
        {
            str_ += std::string(indent_ * 2u, ' ');
            was_newline_ = false;
        }
        str_ += tokens.c_str();
    }

    void do_write_newline() override
    {
        str_ += "\n";
        was_newline_ = true;
    }

    std::string        str_;
    generation_options options_;
    unsigned           indent_      = 0;
    bool               was_newline_ = false;
};

inline std::string get_code(const cppast::cpp_entity&                  e,
                            cppast::code_generator::generation_options options = {})
{
    test_generator generator(options);
    cppast::generate_code(generator, e);
    auto str = generator.str();
    if (!str.empty() && str.back() == '\n')
        str.pop_back();
    return str;
}

template <typename Func, typename T>
auto visit_callback(bool, Func f, const T& t) -> decltype(f(t) == true)
{
    return f(t);
}

template <typename Func, typename T>
bool visit_callback(int check, Func f, const T& t)
{
    f(t);
    return check == 1;
}

template <typename T, typename Func>
unsigned test_visit(const cppast::cpp_file& file, Func f, bool check_code = true)
{
    auto count = 0u;
    cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (info.event == cppast::visitor_info::container_entity_exit)
            return true; // already handled

        if (e.kind() == T::kind())
        {
            auto& obj       = static_cast<const T&>(e);
            auto  check_cur = visit_callback(check_code, f, obj);
            ++count;

            if (check_cur)
            {
                INFO(e.name());
                REQUIRE(e.comment());
                REQUIRE(e.comment().value() == get_code(e));
            }
        }

        return true;
    });

    return count;
}

// number of direct children
template <class Entity>
unsigned count_children(const Entity& cont)
{
    return unsigned(std::distance(cont.begin(), cont.end()));
}

// ignores templated scopes
inline std::string full_name(const cppast::cpp_entity& e)
{
    if (e.name().empty())
        return "";
    else if (cppast::is_parameter(e.kind()))
        // parameters don't have a full name
        return e.name();

    std::string scopes;

    for (auto cur = e.parent(); cur; cur = cur.value().parent())
        // prepend each scope, if there is any
        type_safe::with(cur.value().scope_name(), [&](const cppast::cpp_scope_name& cur_scope) {
            scopes = cur_scope.name() + "::" + scopes;
        });

    if (e.kind() == cppast::cpp_entity_kind::class_t)
    {
        auto& c = static_cast<const cppast::cpp_class&>(e);
        return scopes + c.semantic_scope() + c.name();
    }
    else
        return scopes + e.name();
}

// checks the full name/parent
inline void check_parent(const cppast::cpp_entity& e, const char* parent_name,
                         const char* full_name)
{
    REQUIRE(e.parent());
    REQUIRE(e.parent().value().name() == parent_name);
    REQUIRE(::full_name(e) == full_name);
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
    case cpp_expression_kind::unexposed_t:
        return static_cast<const cpp_unexposed_expression&>(parsed).expression().as_string()
               == static_cast<const cpp_unexposed_expression&>(synthesized)
                      .expression()
                      .as_string();

    case cpp_expression_kind::literal_t:
        return static_cast<const cpp_literal_expression&>(parsed).value()
               == static_cast<const cpp_literal_expression&>(synthesized).value();
    }

    return false;
}

template <typename T, class Predicate>
bool equal_ref(const cppast::cpp_entity_index&                   idx,
               const cppast::basic_cpp_entity_ref<T, Predicate>& parsed,
               const cppast::basic_cpp_entity_ref<T, Predicate>& synthesized,
               const char*                                       full_name_override = nullptr)
{
    if (parsed.name() != synthesized.name())
        return false;
    else if (parsed.is_overloaded() != synthesized.is_overloaded())
        return false;
    else if (parsed.is_overloaded())
        return false;

    auto entities = parsed.get(idx);
    if (entities.size() != 1u)
        return false;
    return entities[0u]->name().empty()
           || full_name(*entities[0u]) == (full_name_override ? full_name_override : parsed.name());
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
