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

    if(lexer.read_next_token())
    {
        if(lexer.current_token().kind == token::token_kind::paren_open)
        {
            node_list args;

            bool error = false, finished = false;
            std::shared_ptr<ast_node> arg = nullptr;

            if(lexer.buffer_size() > 0 &&
               lexer.next_token(0).kind == token::token_kind::paren_close)
            {
                // eat paren
                lexer.read_next_token();

                finished = true;
            }

            while(!finished && !error)
            {
                arg = parse_expression(lexer);

                if(arg != nullptr)
                {
                    args.push_back(arg);

                    // try to read a comma or a cosing paren,
                    // else error
                    if(!lexer.read_next_token())
                    {
                        error = true;
                    }
                    else if(!error && lexer.current_token().kind == token::token_kind::paren_close)
                    {
                        finished = true;
                    }
                    else if(!error && lexer.current_token().kind != token::token_kind::comma)
                    {
                        // expected ','
                        error = true;
                    }
                }
                else
                {
                    // expected arg expression
                    error = true;
                }
            }

            if(error)
            {
                return nullptr;
            }
            else
            {
                return std::make_shared<ast_expression_invoke>(
                    std::move(callee),
                    std::move(args)
                );
            }
        }
        else
        {
            // expected '('
            return nullptr;
        }
    }
    else
    {
        // expected '('
        return nullptr;
    }
}

std::shared_ptr<ast_identifier> parse_identifier(buffered_lexer& lexer)
{
    std::vector<std::string> scope_names;

    while(lexer.buffer_size() > 0 &&
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
    }

    if(scope_names.empty())
    {
        return nullptr;
    }
    else
    {
        return std::make_shared<ast_identifier>(std::move(scope_names));
    }
}

}

}

}
