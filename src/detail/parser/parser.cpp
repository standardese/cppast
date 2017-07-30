// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/parser.hpp>

using namespace cppast;
using namespace cppast::detail::parser;

parser::parser(lexer& lexer) :
    _lexer{lexer, 10}
{}

const diagnostic_logger& parser::logger() const
{
    return _lexer.logger();
}

std::shared_ptr<ast_expression_invoke> parser::parse_invoke()
{
    auto result = do_parse_invoke();

    if(result != nullptr && _lexer.buffer_size() > 0)
    {
        logger().log("parser.parse_invoke", severity::warning, source_location::make_unknown(),
            "Unexpected token \"" + _lexer.next_token(0).string_value() + "\" after invoke expression");
    }

    return result;
}

std::shared_ptr<ast_expression_cpp_attribute> parser::parse_cpp_attribute()
{
    auto result = do_parse_cpp_attribute();

    if(result != nullptr && _lexer.buffer_size() > 0)
    {
        logger().log("parser.parse_cpp_attribute", severity::warning, source_location::make_unknown(),
            "Unexpected token \"" + _lexer.next_token(0).string_value() + "\" after C++ attribute expression");
    }

    return result;
}

std::shared_ptr<ast_node> parser::do_parse_expression()
{
    std::shared_ptr<ast_node> expr = nullptr;

    if(_lexer.buffer_size() > 0)
    {
        if(_lexer.next_token(0).kind == token::token_kind::paren_open)
        {
            // eat paren
            _lexer.read_next_token();

            expr = do_parse_expression();

            if(_lexer.read_next_token() &&
               _lexer.current_token().kind != token::token_kind::paren_close)
            {
                logger().log("parser.expr", severity::error, source_location::make_unknown(),
                    "Expected \")\" after expression");
                return nullptr;
            }
        }
        else
        {
            if(_lexer.buffer_size() > 0)
            {
                // Use the next token kind to guess the right
                // grammar resolution
                switch(_lexer.next_token(0).kind)
                {
                case token::token_kind::identifier:
                    return do_parse_invoke();
                case token::token_kind::string_literal:
                case token::token_kind::int_iteral:
                case token::token_kind::float_literal:
                {
                    if(_lexer.read_next_token())
                    {
                        return literal(_lexer.current_token());
                    }
                    else
                    {
                        logger().log("parser.expr", severity::error, source_location::make_unknown(),
                            "Expected literal");
                        return nullptr;
                    }
                }
                default:
                    logger().log("parser.expr", severity::error, source_location::make_unknown(),
                        "Unexpected token \"" + _lexer.next_token(0).string_value() + "\"");
                    return nullptr;
                }
            }
            else
            {
                logger().log("parser.expr", severity::error, source_location::make_unknown(),
                    "Expected token after \"" + _lexer.current_token().string_value() + "\"");
                return nullptr;
            }
        }
    }

    return expr;
}

std::shared_ptr<ast_expression_invoke> parser::do_parse_invoke()
{
    auto callee = do_parse_identifier();

    if(callee == nullptr)
    {
        logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
            "Expected identifier, got \"" + _lexer.current_token().string_value() + "\"");
        return nullptr;
    }

    if(_lexer.buffer_size() > 0)
    {
        if(_lexer.next_token(0).kind == token::token_kind::paren_open)
        {
            auto args_result = do_parse_arguments();

            if(args_result.first)
            {
                return std::make_shared<ast_expression_invoke>(
                    std::move(callee),
                    std::move(args_result.second)
                );
            }
            else
            {
                logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
                    "Expected arguments after \"(\"");
                return nullptr;
            }
        }
        else
        {
            // no parens is interpreted as call without args
            return std::make_shared<ast_expression_invoke>(
                std::move(callee),
                node_list{}
            );
        }
    }
    else
    {
        logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
            "Expected \"(\"");
        return nullptr;
    }
}

std::pair<bool, node_list> parser::do_parse_arguments(const token::token_kind open_delim, const token::token_kind close_delim)
{
    node_list args;
    bool finished = false, error = false;

    if(!_lexer.read_next_token() ||
       _lexer.current_token().kind != open_delim)
    {
        logger().log("parser.invoke_args", severity::error, source_location::make_unknown(),
            std::string("Expected ") + to_string(open_delim) + ", got " + to_string(_lexer.current_token().kind)
            + "(\"" + _lexer.current_token().string_value() + "\")");
        return std::make_pair(false, std::move(args));
    }

    while(!error && !finished)
    {
        auto arg = do_parse_expression();

        if(arg != nullptr)
        {
            args.push_back(std::move(arg));
        }

        if(_lexer.buffer_size() > 0)
        {
            if(_lexer.next_token(0).kind == close_delim)
            {
                finished = true;

                // eat closing token
                _lexer.read_next_token();
            }
            else if(_lexer.next_token(0).kind == token::token_kind::comma)
            {
                // eat comma
                _lexer.read_next_token();
            }
            else
            {
                // expected comma or closing token
                logger().log("parser.invoke_args", severity::error, source_location::make_unknown(),
                    std::string("Expected ") + to_string(open_delim) + "or comma, got " + to_string(_lexer.next_token(0).kind)
                    + "(\"" + _lexer.next_token(0).string_value() + "\")");
                error = true;
            }
        }
        else
        {
            logger().log("parser.invoke_args", severity::error, source_location::make_unknown(),
                std::string("Expected ") + to_string(open_delim) + "or comma, got " + to_string(_lexer.next_token(0).kind)
                + "(\"" + _lexer.next_token(0).string_value() + "\")");
            error = true;
        }
    }

    return std::make_pair(!error, std::move(args));
}

std::shared_ptr<ast_identifier> parser::do_parse_identifier()
{
    std::vector<std::string> scope_names;
    bool finished = false, error = false;

    while(!finished && !error)
    {
        if(_lexer.buffer_size() > 0 &&
           _lexer.next_token(0).kind == token::token_kind::identifier)
        {
            // eat the identifier
            _lexer.read_next_token();

            scope_names.push_back(_lexer.current_token().string_value());

            if(_lexer.buffer_size() > 0 &&
               _lexer.next_token(0).kind == token::token_kind::double_colon)
            {
                // eat the double colon
                _lexer.read_next_token();
            }
            else
            {
                finished = true;
            }
        }
        else
        {
            if(_lexer.buffer_size() > 0)
            {
                logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
                    "Expected identifier, got \"" + _lexer.current_token().string_value() + "\"");
            }
            else
            {
                logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
                    "Expected identifier");
            }

            error = true;
        }
    }

    if(error || scope_names.empty())
    {
        if(!error)
        {
            logger().log("parser.invoke_expr", severity::error, source_location::make_unknown(),
                "Expected identifier");
        }

        return nullptr;
    }
    else
    {
        return std::make_shared<ast_identifier>(std::move(scope_names));
    }
}

std::shared_ptr<ast_expression_cpp_attribute> parser::do_parse_cpp_attribute()
{
    if(_lexer.read_next_token())
    {
        if(_lexer.current_token().kind == token::token_kind::double_bracket_open)
        {
            auto body = do_parse_invoke();

            if(body == nullptr)
            {
                // expected body expression
                logger().log("parser.cpp_attribute", severity::error, source_location::make_unknown(),
                    "Expected attribute body");
                return nullptr;
            }

            if(_lexer.read_next_token() && _lexer.current_token().kind == token::token_kind::double_bracket_close)
            {
                return std::make_shared<ast_expression_cpp_attribute>(std::move(body));
            }
            else
            {
                logger().log("parser.cpp_attribute", severity::error, source_location::make_unknown(),
                    "Expected ]]");
                return nullptr;
            }
        }
        else
        {
            logger().log("parser.cpp_attribute", severity::error, source_location::make_unknown(),
                "Expected [[, got \"{}\" ({})", _lexer.current_token().string_value(), _lexer.current_token().kind);
            return nullptr;
        }
    }
    else
    {
        logger().log("parser.cpp_attribute", severity::error, source_location::make_unknown(),
            "Expected [[, got nothing");
        return nullptr;
    }
}
