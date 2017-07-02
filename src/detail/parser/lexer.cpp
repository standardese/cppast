#include <cppast/detail/parser/lexer.hpp>
#include <cppast/detail/assert.hpp>
#include <cstdlib>
#include <unordered_map>

using namespace cppast::detail::parser;

std::size_t token::length() const
{
    return token.length();
}

const std::string& token::string_value() const
{
    return token;
}

long long token::int_value() const
{
    return std::atoll(token.c_str());
}

unsigned long long token::uint_value() const
{
    char* end;
    return std::strtoull(token.c_str(), &end, 10);
}

double token::float_value() const
{
    return std::atof(token.c_str());
}

lexer::lexer(std::istream& input) :
    _input(input),
    _pos{0},
    _last_char{' '}
{}

const token& lexer::current_token() const
{
    return _current_token;
}

std::size_t lexer::current_pos() const
{
    return _pos;
}

bool is_space(char c)
{
    return c == ' ';
}

struct key_token_match_info
{
    char c;
    token::token_kind kind;
    std::size_t count;
};

bool lexer::eof() const
{
    return _input.eof() || !_input.good();
}

bool lexer::read_next_token()
{
    _token_buffer.str("");

    if(!good() || !_input.good())
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
                    set_error(_pos) << "Unexpected floating point dot after \""
                        << _token_buffer.str() << "\"";
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

lexer::error_builder::error_builder(lexer* self, std::size_t begin, std::size_t end) :
    self{self},
    begin{begin},
    end{end}
{
    stream << "At " << begin << ": ";
}

lexer::error_builder::~error_builder()
{
    self->_error = type_safe::make_optional(
        lexer::error_info{
            stream.str(),
            begin,
            end
        }
    );
}

lexer::error_builder lexer::set_error(std::size_t begin, std::size_t end)
{
    return {this, begin, end};
}

lexer::error_builder lexer::set_error(std::size_t begin)
{
    return {this, begin, begin};
}

bool lexer::good() const
{
    return !_error.has_value();
}

const type_safe::optional<lexer::error_info>& lexer::error() const
{
    return _error;
}

void lexer::save_token(const token::token_kind kind, const std::string& token)
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
}

namespace cppast
{

namespace detail
{

namespace parser
{

std::ostream& operator<<(std::ostream& os, const token::token_kind kind)
{
    switch(kind)
    {
    case token::token_kind::identifier:
        return os << "identifier";
    case token::token_kind::string_literal:
        return os << "string_literal";
    case token::token_kind::int_iteral:
        return os << "int_literal";
    case token::token_kind::unint_literal:
        return os << "unint_literal";
    case token::token_kind::float_literal:
        return os << "float_literal";
    case token::token_kind::bool_literal:
        return os << "bool_literal";
    case token::token_kind::double_colon:
        return os << "double_colon";
    case token::token_kind::comma:
        return os << "comma";
    case token::token_kind::semicolon:
        return os << "semicolon";
    case token::token_kind::bracket_open:
        return os << "bracket_open";
    case token::token_kind::bracket_close:
        return os << "bracket_close";
    case token::token_kind::double_bracket_open:
        return os << "double_bracket_open";
    case token::token_kind::double_bracket_close:
        return os << "double_bracket_close";
    case token::token_kind::paren_open:
        return os << "paren_open";
    case token::token_kind::paren_close:
        return os << "paren_close";
    case token::token_kind::angle_bracket_open:
        return os << "angle_bracket_open";
    case token::token_kind::angle_bracket_close:
        return os << "angle_bracket_close";
    default:
        return os << "'" << static_cast<char>(kind) << "'";
    }
}

std::ostream& operator<<(std::ostream& os, const token& token)
{
    return os << "token{\"" << token.token << "\", kind: "
        << token.kind << ", column: " << token.column << "}";
}

}

}

}
