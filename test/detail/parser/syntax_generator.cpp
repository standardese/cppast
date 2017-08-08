// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "syntax_generator.hpp"
#include <unordered_set>

using namespace cppast::test;
using token = cppast::detail::parser::token;
using ast = syntax_generator::ast;
template<typename Node>
using syntax = syntax_generator::syntax<Node>;
using invoke_args_syntax = syntax_generator::invoke_args_syntax;

syntax<ast::terminal_string> syntax_generator::random_terminal_string()
{
    auto token = token_generator.random_token(token::token_kind::string_literal);

    return {
        std::make_shared<ast::terminal_string>(token.string_value(), token),
        {token}
    };
}

syntax<ast::terminal_integer> syntax_generator::random_terminal_integer()
{
    auto token = token_generator.random_token(token::token_kind::int_iteral);

    return {
        std::make_shared<ast::terminal_integer>(token.int_value(), token),
        {token}
    };
}

syntax<ast::terminal_float> syntax_generator::random_terminal_float()
{
    auto token = token_generator.random_token(token::token_kind::float_literal);

    return {
        std::make_shared<ast::terminal_float>(token.float_value(), token),
        {token}
    };
}

syntax<ast::identifier> syntax_generator::random_identifier(std::size_t scopes)
{
    std::vector<std::string> scope_names;
    std::vector<token> tokens;

    for(std::size_t i = 0; i < scopes; ++i)
    {
        auto scope_name_token = token_generator.random_token(token::token_kind::identifier);
        scope_names.push_back(scope_name_token.string_value());
        tokens.push_back(scope_name_token);

        if( i < scopes - 1)
        {
            tokens.push_back(token_generator.random_token(token::token_kind::double_colon));
        }
    }

    return std::make_pair(
        std::make_shared<ast::identifier>(std::move(scope_names)),
        std::move(tokens)
    );
}

syntax<ast::identifier> syntax_generator::random_full_qualified_identifier(std::size_t scopes)
{
    auto unqualified = random_identifier(scopes);

    // Note the ast node keeps untouched, the same stream of tokens
    // prefixed with a double colon should yield the same identifier
    unqualified.second.insert(
        unqualified.second.begin(),
        token_generator.random_token(token::token_kind::double_colon)
    );

    return unqualified;
}

std::vector<token> syntax_generator::build_invoke_args(const std::vector<syntax<ast::node>>& args)
{
    std::vector<token> result;
    result.push_back(token_generator.random_token(token::token_kind::paren_open));

    for(const auto& arg : args)
    {
        for(const auto& token : arg.second)
        {
            result.push_back(token);
        }

        result.push_back(token_generator.random_token(token::token_kind::comma));
    }

    // Remove the comma after the last arg, if any
    if(result.back().kind == token::token_kind::comma)
    {
        result.pop_back();
    }

    result.push_back(token_generator.random_token(token::token_kind::paren_close));

    return result;
}

syntax<ast::expression_invoke> syntax_generator::random_expression_invoke(std::size_t total_args, std::size_t depth)
{
    auto name = random_identifier();
    auto args = random_invoke_args(total_args, depth);

    std::vector<token> tokens{name.second};

    std::copy(args.second.begin(), args.second.end(),
        std::back_inserter(tokens));

    return {
        std::make_shared<ast::expression_invoke>(std::move(name.first), std::move(args.first)),
        std::move(tokens)
    };
}

syntax<ast::expression_cpp_attribute> syntax_generator::random_expression_cpp_attribute(std::size_t total_args, std::size_t depth)
{
    auto body = random_expression_invoke(total_args, depth);

    body.second.insert(
        body.second.begin(),
        token_generator.random_token(token::token_kind::double_bracket_open)
    );
    body.second.push_back(
        token_generator.random_token(token::token_kind::double_bracket_close)
    );

    return {
        std::make_shared<ast::expression_cpp_attribute>(body.first),
        std::move(body.second)
    };
}

syntax<ast::node> syntax_generator::random_node(ast::node::node_kind kind, std::size_t args, std::size_t depth)
{
    switch(kind)
    {
    case ast::node::node_kind::unespecified:
        return {nullptr, {}};
    case ast::node::node_kind::terminal_string:
        return random_terminal_string();
    case ast::node::node_kind::terminal_integer:
        return random_terminal_integer();
    case ast::node::node_kind::terminal_float:
        return random_terminal_float();
    case ast::node::node_kind::identifier:
        return random_identifier(token_generator.random_int(0, 100));
    case ast::node::node_kind::expression_invoke:
        return random_expression_invoke(args, depth);
    case ast::node::node_kind::expression_cpp_attribute:
        return random_expression_cpp_attribute(args, depth);
    }
}

syntax<ast::node> syntax_generator::random_invoke_arg(std::size_t depth)
{
    static ast::node::node_kind supported_kinds[] = {
        ast::node::node_kind::terminal_string,
        ast::node::node_kind::terminal_integer,
        ast::node::node_kind::terminal_float,
        ast::node::node_kind::expression_invoke
    };

    std::size_t index = token_generator.random_int<std::size_t>(0, sizeof(supported_kinds)/sizeof(ast::node::node_kind) - 1 - (depth == 0));
    auto node_kind = supported_kinds[index];

    depth = (depth == 0 ? 0 : depth - 1);
    return random_node(node_kind, token_generator.random_int(0, 4), depth);
}

invoke_args_syntax syntax_generator::random_invoke_args(std::size_t count, std::size_t depth)
{
    std::vector<std::shared_ptr<ast::node>> args;
    args.reserve(count);
    std::vector<syntax<ast::node>> random_args;
    random_args.reserve(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        random_args.push_back(random_invoke_arg(depth));
        args.push_back(random_args.back().first);
    }

    std::vector<token> tokens{build_invoke_args(random_args)};

    return {
        std::move(args),
        std::move(tokens)
    };
}
