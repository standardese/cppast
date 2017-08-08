// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED
#define CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED

#include <cppast/detail/parser/ast.hpp>
#include "token_generator.hpp"

namespace cppast
{

namespace test
{

class syntax_generator
{
public:
    struct ast
    {
        using node = cppast::detail::parser::ast::node;
        using terminal_string = cppast::detail::parser::ast::terminal_string;
        using terminal_integer = cppast::detail::parser::ast::terminal_integer;
        using terminal_float = cppast::detail::parser::ast::terminal_float;
        using identifier = cppast::detail::parser::ast::identifier;
        using expression_invoke = cppast::detail::parser::ast::expression_invoke;
        using expression_cpp_attribute = cppast::detail::parser::ast::expression_cpp_attribute;
    };

    template<typename Node>
    using syntax = std::pair<std::shared_ptr<Node>, std::vector<cppast::detail::parser::token>>;

    using invoke_args_syntax = std::pair<
        std::vector<std::shared_ptr<ast::node>>,
        std::vector<cppast::detail::parser::token>
    >;

    syntax<ast::terminal_string> random_terminal_string();
    syntax<ast::terminal_integer> random_terminal_integer();
    syntax<ast::terminal_float> random_terminal_float();
    syntax<ast::identifier> random_identifier(std::size_t scopes = 1);
    syntax<ast::identifier> random_full_qualified_identifier(std::size_t scopes = 1);

    syntax<ast::expression_invoke> random_expression_invoke(std::size_t args, std::size_t depth);
    syntax<ast::expression_cpp_attribute> random_expression_cpp_attribute(std::size_t args, std::size_t depth);

    template<typename... Args>
    std::vector<cppast::detail::parser::token> build_invoke_args(const Args&... args)
    {
        // upcast all args to shared_ptr<ast::node>
        return build_invoke_args(std::vector<syntax<ast::node>>{
            std::shared_ptr<ast::node>{args, args.get()}...
        });
    }

    std::vector<cppast::detail::parser::token> build_invoke_args(const std::vector<syntax<ast::node>>& args);

    syntax<ast::node> random_node(ast::node::node_kind kind, std::size_t args = 0, std::size_t depth = 0);

    syntax<ast::node> random_invoke_arg(std::size_t depth);
    invoke_args_syntax random_invoke_args(std::size_t count, std::size_t depth);

private:
    cppast::test::token_generator token_generator;
};

} // namespace cppast::test

} // namespace cppast

#endif // CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED
