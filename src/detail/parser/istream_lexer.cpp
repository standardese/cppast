// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/istream_lexer.hpp>
#include <cppast/detail/assert.hpp>
#include <unordered_map>

using namespace cppast;
using namespace cppast::detail::parser;

istream_lexer::istream_lexer(std::istream& input, const diagnostic_logger& logger) :
    lexer{logger},
    _input(input),
    _pos{0},
    _last_char{' '}
{}

const token& istream_lexer::current_token() const
{
    return _current_token;
}

bool is_space(char c)
{
    return c == ' ' || c == '\n';
}

struct key_token_match_info
{
    char c;
    token::token_kind kind;
    std::size_t count;
};

bool istream_lexer::eof() const
{
    return _input.eof() || !_input.good();
}

bool istream_lexer::good() const
{
    return !_logger.error_logged();
}

bool istream_lexer::read_next_token()
{
    _token_buffer.str("");

    if(!_input.good())
    {
        return false;
    }

    _input.get(_last_char);
    _pos++;

    while(is_space(_last_char) &&
          _input.get(_last_char))
    {
        _pos++;
    }

    if(!_input.good())
    {
        return false;
    }

    if(_last_char == '"')
    {
        while(_input.get(_last_char) && _last_char != '"')
        {
            _token_buffer.put(_last_char);
            _pos++;
        }

        save_token(token::token_kind::string_literal);
        return true;
    }

    if(std::isalpha(_last_char))
    {
        _token_buffer.put(_last_char);

        while(_input.get(_last_char) && (std::isalnum(_last_char) ||
                _last_char == '_'))
        {
            _token_buffer.put(_last_char);
            _pos++;
        }

        if(_input.good())
        {
            _input.putback(_last_char);
        }

        save_token(token::token_kind::identifier);
        return true;
    }

    if(std::isdigit(_last_char))
    {
        bool dot = false;
        _token_buffer.put(_last_char);

        while(_input.get(_last_char) && (std::isdigit(_last_char) ||
                _last_char == '.'))
        {
            _pos++;

            if(_last_char == '.')
            {
                if(!dot)
                {
                    _token_buffer.put(_last_char);
                    dot = true;
                }
                else
                {
                    _logger.log("lexer", severity::error, source_location::make_unknown(),
                        "Unexpected floating point dot after \"{}\"",
                        _token_buffer.str()
                    );
                    return false;
                }
            }
            else
            {
                _token_buffer.put(_last_char);
            }
        }

        save_token(dot ? token::token_kind::float_literal : token::token_kind::int_iteral);

        if(!std::isdigit(_last_char))
        {
            // Put the character that finished with number parsing
            // back into the input for further processing. Else we're
            // consuming one extra char
            _input.putback(_last_char);
        }

        return true;
    }

    auto handle_simple_token = [&](char expected, const token::token_kind kind)
    {
        if(_last_char == expected)
        {
            save_token(kind, {expected});
            DEBUG_ASSERT(_token_buffer.str().empty(), detail::assert_handler{});
            return true;
        }
        else
        {
            return false;
        }
    };

    auto handle_double_token = [&](char expected, const token::token_kind kind)
    {
        if(_last_char == expected)
        {
            if(_input.get(_last_char) && _last_char == expected)
            {
                _pos++;
                save_token(kind, std::string{{expected, expected}});
                DEBUG_ASSERT(_token_buffer.str().empty(), detail::assert_handler{});
                return true;
            }
            else
            {
                if(_input.good())
                {
                    // De-eat the last character
                    _input.putback(_last_char);
                    _last_char = expected;
                }
                return false;
            }
        }
        else
        {
            return false;
        }
    };

    if(handle_double_token('[', token::token_kind::double_bracket_open))
    {
        return true;
    }

    if(handle_double_token(']', token::token_kind::double_bracket_close))
    {
        return true;
    }

    if(handle_double_token(':', token::token_kind::double_colon))
    {
        return true;
    }

    if(handle_simple_token(',', token::token_kind::comma))
    {
        return true;
    }

    if(handle_simple_token('(', token::token_kind::paren_open))
    {
        return true;
    }

    if(handle_simple_token(')', token::token_kind::paren_close))
    {
        return true;
    }

    if(handle_simple_token('[', token::token_kind::bracket_open))
    {
        return true;
    }

    if(handle_simple_token(']', token::token_kind::bracket_close))
    {
        return true;
    }

    if(handle_simple_token('<', token::token_kind::angle_bracket_open))
    {
        return true;
    }

    if(handle_simple_token('>', token::token_kind::angle_bracket_close))
    {
        return true;
    }

    save_token(static_cast<token::token_kind>(_last_char), {_last_char});
    return true;
}

void istream_lexer::save_token(const token::token_kind kind, const std::string& token)
{
    if(token.empty())
    {
        _current_token.token = _token_buffer.str();
    }
    else
    {
        _current_token.token = token;
    }

    _current_token.kind = kind;
    _current_token.column = _pos - _current_token.token.length();

    logger().log("istream_lexer.save_token", severity::debug, source_location::make_unknown(),
        "token {} saved", _current_token);
}
