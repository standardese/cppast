#include <cppast/detail/parser/parser.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

std::shared_ptr<ast_node> parse_expression(buffered_lexer& lexer)
{
    std::shared_ptr<ast_node> expr = nullptr;

    if(lexer.buffer_size() > 0)
    {
        if(lexer.next_token(0).kind == token::token_kind::paren_open)
        {
            // eat paren
            lexer.read_next_token();

            expr = parse_expression(lexer);

            if(lexer.read_next_token() &&
               lexer.current_token().kind != token::token_kind::paren_close)
            {
                // Expected ')'
                return nullptr;
            }
        }
        else
        {
            if(lexer.buffer_size() > 0)
            {
                // Use the next token kind to guess the right
                // grammar resolution
                switch(lexer.next_token(0).kind)
                {
                case token::token_kind::identifier:
                    return parse_invoke(lexer);
                case token::token_kind::string_literal:
                case token::token_kind::int_iteral:
                case token::token_kind::float_literal:
                {
                    if(lexer.read_next_token())
                    {
                        return literal(lexer.current_token());
                    }
                    else
                    {
                        // Error eating the next token
                        return nullptr;
                    }
                }
                default:
                    // unexpected token
                    return nullptr;
                }
            }
            else
            {
                // Cannot guess
                return nullptr;
            }
        }
    }

    return expr;
}

std::shared_ptr<ast_expression_invoke> parse_invoke(buffered_lexer& lexer)
{
    auto callee = parse_identifier(lexer);

    if(callee == nullptr)
    {
        // Expected identifier
        return nullptr;
    }

    if(lexer.buffer_size() > 0)
    {
        if(lexer.next_token(0).kind == token::token_kind::paren_open)
        {
            auto args_result = parse_arguments(lexer);

            if(args_result.first)
            {
                return std::make_shared<ast_expression_invoke>(
                    std::move(callee),
                    std::move(args_result.second)
                );
            }
            else
            {
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
        // expected '('
        return nullptr;
    }
}

std::pair<bool, node_list> parse_arguments(buffered_lexer& lexer, const token::token_kind open_delim, const token::token_kind close_delim)
{
    node_list args;
    bool finished = false, error = false;

    if(!lexer.read_next_token() ||
       lexer.current_token().kind != open_delim)
    {
        // expected open delimiter
        return std::make_pair(false, std::move(args));
    }

    while(!error && !finished)
    {
        auto arg = parse_expression(lexer);

        if(arg != nullptr)
        {
            args.push_back(std::move(arg));
        }

        if(lexer.buffer_size() > 0)
        {
            if(lexer.next_token(0).kind == close_delim)
            {
                finished = true;

                // eat closing token
                lexer.read_next_token();
            }
            else if(lexer.next_token(0).kind == token::token_kind::comma)
            {
                // eat comma
                lexer.read_next_token();
            }
            else
            {
                // expected comma or closing token
                error = true;
            }
        }
        else
        {
            // expected comma or closing token
            error = true;
        }
    }

    return std::make_pair(!error, std::move(args));
}

std::shared_ptr<ast_identifier> parse_identifier(buffered_lexer& lexer)
{
    std::vector<std::string> scope_names;
    bool finished = false, error = false;

    while(!finished && !error)
    {
        if(lexer.buffer_size() > 0 &&
           lexer.next_token(0).kind == token::token_kind::identifier)
        {
            // eat the identifier
            lexer.read_next_token();

            scope_names.push_back(lexer.current_token().string_value());

            if(lexer.buffer_size() > 0 &&
               lexer.next_token(0).kind == token::token_kind::double_colon)
            {
                // eat the double colon
                lexer.read_next_token();
            }
            else
            {
                finished = true;
            }
        }
        else
        {
            // expected identifier
            error = true;
        }
    }

    if(error || scope_names.empty())
    {
        return nullptr;
    }
    else
    {
        return std::make_shared<ast_identifier>(std::move(scope_names));
    }
}

std::shared_ptr<ast_expression_cpp_attribute> parse_cpp_attribute(buffered_lexer& lexer)
{
    auto expectToken = [&lexer](token::token_kind kind, std::size_t times)
    {
        for(std::size_t i = 0; i < times; ++i)
        {
            if(!lexer.read_next_token() ||
               lexer.current_token().kind != kind)
            {
                return false;
            }
        }

        return true;
    };

    if(!expectToken(token::token_kind::double_bracket_open, 1))
    {
        // expected [[
        return nullptr;
    }

    auto body = parse_invoke(lexer);

    if(body == nullptr)
    {
        // expected body expression
        return nullptr;
    }

    if(!expectToken(token::token_kind::double_bracket_close, 1))
    {
        // expected ]]
        return nullptr;
    }

    return std::make_shared<ast_expression_cpp_attribute>(std::move(body));
}

}

}

}
