// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/parser.hpp>

using namespace cppast;
using namespace cppast::detail::parser;
using namespace cppast::detail::parser::ast;

parser::parser(lexer& lexer) :
    _lexer{lexer, 3}
{}

const diagnostic_logger& parser::logger() const
{
    return _lexer.logger();
}

std::shared_ptr<expression_invoke> parser::parse_invoke()
{
    auto result = do_parse_invoke();

    if(result != nullptr && _lexer.buffer_size() > 0)
    {
        logger().log("parser.parse_invoke", severity::warning, _lexer.location(),
            "Unexpected token \"" + _lexer.next_token(0).token + "\" after invoke expression");
    }

    return result;
}

std::shared_ptr<expression_cpp_attribute> parser::parse_cpp_attribute()
{
    auto result = do_parse_cpp_attribute();

    if(result != nullptr && _lexer.buffer_size() > 0)
    {
        logger().log("parser.parse_cpp_attribute", severity::warning, _lexer.location(),
            "Unexpected token \"" + _lexer.next_token(0).token + "\" after C++ attribute expression");
    }

    return result;
}

std::shared_ptr<node> parser::do_parse_expression()
{
    std::shared_ptr<node> expr = nullptr;

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
                logger().log("parser.expr", severity::error, _lexer.location(),
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
                        return make_literal(_lexer.current_token());
                    }
                    else
                    {
                        logger().log("parser.expr", severity::error, _lexer.location(),
                            "Expected literal");
                        return nullptr;
                    }
                }
                default:
                    logger().log("parser.expr", severity::error, _lexer.location(),
                        "Unexpected token {}", _lexer.next_token(0));
                    return nullptr;
                }
            }
            else
            {
                logger().log("parser.expr", severity::error, _lexer.location(),
                    "Expected token after \"" + _lexer.current_token().string_value() + "\"");
                return nullptr;
            }
        }
    }

    return expr;
}

std::shared_ptr<expression_invoke> parser::do_parse_invoke()
{
    auto callee = do_parse_identifier();

    if(callee == nullptr)
    {
        logger().log("parser.invoke_expr", severity::error, _lexer.location(),
            "Expected callee identifier");
        return nullptr;
    }

    if(_lexer.buffer_size() > 0)
    {
        if(_lexer.next_token(0).kind == token::token_kind::paren_open)
        {
            auto args_result = do_parse_arguments();

            if(args_result.first)
            {
                return std::make_shared<expression_invoke>(
                    std::move(callee),
                    std::move(args_result.second)
                );
            }
            else
            {
                logger().log("parser.invoke_expr", severity::error, _lexer.location(),
                    "Expected arguments after '('");
                return nullptr;
            }
        }
        else
        {
            // no parens is interpreted as call without args
            return std::make_shared<expression_invoke>(
                std::move(callee),
                node_list{}
            );
        }
    }
    else
    {
        // no parens is interpreted as call without args
        return std::make_shared<expression_invoke>(
            std::move(callee),
            node_list{}
        );
    }
}

std::pair<bool, node_list> parser::do_parse_arguments(const token::token_kind open_delim, const token::token_kind close_delim)
{
    node_list args;
    bool finished = false, error = false;

    if(!_lexer.read_next_token() ||
       _lexer.current_token().kind != open_delim)
    {
        logger().log("parser.invoke_args", severity::error, _lexer.location(),
            "Expected '{}'", open_delim);
        return std::make_pair(false, std::move(args));
    }

    if(_lexer.buffer_size() > 0 &&
       _lexer.next_token(0).kind == close_delim)
    {
        // eat closing token
        _lexer.read_next_token();
    }
    else
    {
        while(!error && !finished)
        {
            auto arg = do_parse_expression();

            if(arg != nullptr)
            {
                args.push_back(std::move(arg));

                if(_lexer.read_next_token())
                {
                    if(_lexer.current_token().kind == close_delim)
                    {
                        finished = true;
                    }
                    else if(_lexer.current_token().kind == token::token_kind::comma)
                    {
                        // nop, continue reading args
                    }
                    else
                    {
                        // expected comma or closing token
                        logger().log("parser.invoke_args", severity::error, _lexer.location(),
                            "Expected '{}' or comma (','), got \"{}\"", close_delim, _lexer.current_token().token);
                        error = true;
                    }
                }
                else
                {
                    logger().log("parser.invoke_args", severity::error, _lexer.location(),
                        "Expected '{}' or comma (','), but no more tokens are available", close_delim);
                    error = true;
                }
            }
            else
            {
                logger().log("parser.invoke_args", severity::error, _lexer.location(),
                    "Expected invoke argument");
                error = true;
            }
        }
    }

    return std::make_pair(!error, std::move(args));
}

std::shared_ptr<identifier> parser::do_parse_identifier()
{
    std::vector<std::string> scope_names;
    bool finished = false, error = false;

    if(_lexer.buffer_size() > 0 &&
       _lexer.next_token(0).kind == token::token_kind::double_colon)
    {
        // it seems that the identifier is full qualified, eat the leading
        // double colon
        _lexer.read_next_token();
    }

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
                logger().log("parser.identifier", severity::error, _lexer.location(),
                    "Expected identifier, got \"" + _lexer.next_token(0).token + "\"");
            }
            else
            {
                logger().log("parser.identifier", severity::error, _lexer.location(),
                    "Expected identifier");
            }

            error = true;
        }
    }

    if(error || scope_names.empty())
    {
        if(!error)
        {
            logger().log("parser.invoke_expr", severity::error, _lexer.location(),
                "Expected identifier");
        }

        return nullptr;
    }
    else
    {
        return std::make_shared<identifier>(std::move(scope_names));
    }
}

std::shared_ptr<expression_cpp_attribute> parser::do_parse_cpp_attribute()
{
    if(_lexer.read_next_token())
    {
        if(_lexer.current_token().kind == token::token_kind::double_bracket_open)
        {
            logger().log("parser.cpp_attribute", severity::debug, _lexer.location(),
                "Got {}, continue parsing body (next token is: {})", _lexer.current_token(),
                _lexer.next_token(0));
            auto body = do_parse_invoke();

            if(body == nullptr)
            {
                // expected body expression
                logger().log("parser.cpp_attribute", severity::error, _lexer.location(),
                    "Expected attribute body");
                return nullptr;
            }

            if(_lexer.read_next_token() && _lexer.current_token().kind == token::token_kind::double_bracket_close)
            {
                return std::make_shared<expression_cpp_attribute>(std::move(body));
            }
            else
            {
                logger().log("parser.cpp_attribute", severity::error, _lexer.location(),
                    "Expected ]]");
                return nullptr;
            }
        }
        else
        {
            logger().log("parser.cpp_attribute", severity::error, _lexer.location(),
                "Expected [[, got \"{}\" ({})", _lexer.current_token().string_value(), _lexer.current_token().kind);
            return nullptr;
        }
    }
    else
    {
        logger().log("parser.cpp_attribute", severity::error, _lexer.location(),
            "Expected [[, got nothing");
        return nullptr;
    }
}
