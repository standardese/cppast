// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/istream_lexer.hpp>
#include <cppast/detail/assert.hpp>
#include <unordered_map>
#include <vector>

using namespace cppast;
using namespace cppast::detail::parser;

constexpr istream_lexer::scan_function istream_lexer::scanners[];

istream_lexer::istream_lexer(std::istream& input, const diagnostic_logger& logger, const std::string& filename) :
    lexer{logger},
    _input(input),
    _filename{filename},
    _current_token{
        token::token_kind::unknown,
        "",
        0,
        0
    },
    _column{-1},
    _line{0}, _previous_line_length{0},
    _token_line{0}, _token_column{0},
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

bool istream_lexer::advance()
{
    if(_input.get(_last_char))
    {
        if(_last_char == '\n')
        {
            _line++;
            _previous_line_length = _column;
            _column = -1;
        }
        else
        {
            _column++;
        }

        logger().log("istream_lexer.advance", severity::debug, location(),
            "Moved to {}:{}", _line, _column);

        return true;
    }
    else
    {
        return false;
    }
}

bool istream_lexer::put_back()
{
    if(_input.putback(_last_char))
    {
        if(_column > 0)
        {
            _column--;
        }
        else
        {
            _column = _previous_line_length;
            _previous_line_length = 0;
            _line--;
        }

        logger().log("istream_lexer.put_back", severity::debug, location(),
            "Moved to {}:{}", _line, _column);

        return true;
    }
    else
    {
        return false;
    }
}

istream_lexer::scan_result istream_lexer::scan_string_literal()
{
    if(_last_char == '"')
    {
        save_char(_last_char);

        while(advance() && _last_char != '"')
        {
            save_char(_last_char);
        }

        if(!eof())
        {
            save_char(_last_char);
        }
        else
        {
            logger().log("lexer", severity::error, location(),
                "Expected closing '\"' after \"" + _token_buffer.str() + "\"");
            return scan_result::error;
        }

        save_token(token::token_kind::string_literal);
        return scan_result::successful;
    }
    else
    {
        return scan_result::unsuccessful;
    }
}

istream_lexer::scan_result istream_lexer::scan_identifier()
{
    if(std::isalpha(_last_char))
    {
        save_char(_last_char);

        while(advance() && (std::isalnum(_last_char) ||
                _last_char == '_'))
        {
            save_char(_last_char);
        }

        if(_input.good())
        {
            put_back();
        }

        save_token(token::token_kind::identifier);
        return scan_result::successful;
    }
    else
    {
        return scan_result::unsuccessful;
    }
}

istream_lexer::scan_result istream_lexer::scan_number()
{
    auto is_numeric = [](char c)
    {
        return std::isdigit(c) ||
            c == '+' ||
            c == '-' ||
            c == '.';
    };

    if(is_numeric(_last_char))
    {
        enum class sign_status : char
        {
            no_sign,
            positive = '+',
            negative = '-'
        };

        bool dot;
        sign_status sign;

        switch(_last_char)
        {
        case '.':
            dot = true; sign = sign_status::no_sign; break;
        case '+':
            dot = false; sign = sign_status::positive; break;
        case '-':
            dot = false; sign = sign_status::negative; break;
        default:
            dot = false; sign = sign_status::no_sign; break;
        }

        save_char(_last_char);

        while(advance() && is_numeric(_last_char))
        {
            if(_last_char == '.')
            {
                if(!dot)
                {
                    dot = true;
                }
                else
                {
                    _logger.log("lexer", severity::error, location(),
                        "Unexpected floating point dot after \"{}\"",
                        _token_buffer.str()
                    );
                    return scan_result::error;
                }
            }
            else
            {
                if(_last_char == '+' || _last_char == '-')
                {
                    if(sign == sign_status::no_sign)
                    {
                        sign = static_cast<sign_status>(_last_char);
                    }
                    else
                    {
                        _logger.log("lexer", severity::error, location(),
                            "Unexpected '" + std::string{_last_char} + "' character after \"" +
                            _token_buffer.str() + "\"");
                        return scan_result::error;
                    }
                }
            }

            save_char(_last_char);
        }

        save_token(dot ? token::token_kind::float_literal : token::token_kind::int_iteral);
        return scan_result::successful;
    }
    else
    {
        return scan_result::unsuccessful;
    }
}

istream_lexer::scan_result istream_lexer::scan_samechars_symbol()
{
    static const std::unordered_map<char, std::vector<token::token_kind>> kind_map = {
        {'(', {token::token_kind::paren_open}},
        {')', {token::token_kind::paren_close}},
        {'[', {token::token_kind::bracket_open, token::token_kind::double_bracket_open}},
        {']', {token::token_kind::bracket_close, token::token_kind::double_bracket_close}},
        {'<', {token::token_kind::angle_bracket_open}},
        {'>', {token::token_kind::angle_bracket_close}},
        {':', {token::token_kind::unknown, token::token_kind::double_colon}},
        {',', {token::token_kind::comma}}
    };

    auto it = kind_map.find(_last_char);

    if(it != kind_map.end())
    {
        char target_char = _last_char;
        save_char(_last_char);

        while(advance() && _last_char == target_char)
        {
            save_char(_last_char);
        }

        if(_last_char != target_char)
        {
            put_back();
        }

        const std::string token = _token_buffer.str();
        logger().log("istream_lexer.scan_samechars_symbol", severity::debug, location(),
            "token: " + token);

        auto backtrack = [&]
        {
            for(auto it = token.rbegin(); it != token.rend(); ++it)
            {
                discard_char();
                _last_char = *it;
            }
        };

        if(token.size() <= it->second.size())
        {
            auto kind = it->second[token.size() - 1];

            if(kind != token::token_kind::unknown)
            {
                save_token(kind);
                return scan_result::successful;
            }
            else
            {
                backtrack();
                return scan_result::unsuccessful;
            }
        }
        else
        {
            backtrack();
            return scan_result::unsuccessful;
        }
    }
    else
    {
        return scan_result::unsuccessful;
    }
}

istream_lexer::scan_result istream_lexer::scan_char()
{
    save_char(_last_char);
    save_token(static_cast<token::token_kind>(_last_char));
    return scan_result::successful;
}

bool istream_lexer::read_next_token()
{
    _token_buffer.str("");

    if(!_input.good())
    {
        return false;
    }

    // Read first char in the input
    advance();

    // Discard all leading spaces
    while(is_space(_last_char) &&
          advance());

    // If no more input after consuming
    // spaces, error
    if(!_input.good())
    {
        return false;
    }

    for(auto scanner : scanners)
    {
        switch((this->*scanner)())
        {
        case scan_result::successful:
            return true;
        case scan_result::unsuccessful:
            continue;
        case scan_result::error:
            return false;
        }
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return false;
}

void istream_lexer::save_token(const token::token_kind kind)
{
    _current_token.token = _token_buffer.str();
    _current_token.kind = kind;
    _current_token.line = _token_line;
    _current_token.column = _token_column;

    logger().log("istream_lexer.save_token", severity::debug, location(),
        "token {} saved", _current_token);
}

void istream_lexer::save_char(char c)
{
    if(_token_buffer.str().empty())
    {
        _token_line = _line;
        _token_column = _column;
    }

    _token_buffer.put(c);

    logger().log("istream_lexer.save_char", severity::debug, location(),
        "'{}' saved (token line: {}, token column: {}", c, _token_line, _token_column);
}

void istream_lexer::discard_char()
{
    // kill me
    _token_buffer.seekp(-1, std::ios_base::end);

    put_back();
}

source_location istream_lexer::location() const
{
    return source_location::make_file(_filename, _line, _column);
}
