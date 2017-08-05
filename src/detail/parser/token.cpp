// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/token.hpp>
#include <sstream>
#include <tuple>

using namespace cppast;
using namespace cppast::detail::parser;

std::size_t token::length() const
{
    return token.length();
}

std::string token::string_value() const
{
    return token.substr(1, token.size() - 2);
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

namespace cppast
{

namespace detail
{

namespace parser
{

std::ostream& operator<<(std::ostream& os, const token::token_kind kind)
{
    return os << to_string(kind);
}

std::ostream& operator<<(std::ostream& os, const token& token)
{
    return os << "token{\"" << token.token << "\", kind: "
        << token.kind << ", line: " << token.line
        << ", column: " << token.column << "}";
}
std::string to_string(const token& token)
{
    std::stringstream ss;
    ss << token;
    return ss.str();
}

std::string to_string(token::token_kind kind)
{
    switch(kind)
    {
    case token::token_kind::identifier:
        return "identifier";
    case token::token_kind::string_literal:
        return "string_literal";
    case token::token_kind::int_iteral:
        return "int_literal";
    case token::token_kind::unint_literal:
        return "unint_literal";
    case token::token_kind::float_literal:
        return "float_literal";
    case token::token_kind::bool_literal:
        return "bool_literal";
    case token::token_kind::double_colon:
        return "double_colon";
    case token::token_kind::comma:
        return "comma";
    case token::token_kind::semicolon:
        return "semicolon";
    case token::token_kind::bracket_open:
        return "bracket_open";
    case token::token_kind::bracket_close:
        return "bracket_close";
    case token::token_kind::double_bracket_open:
        return "double_bracket_open";
    case token::token_kind::double_bracket_close:
        return "double_bracket_close";
    case token::token_kind::paren_open:
        return "paren_open";
    case token::token_kind::paren_close:
        return "paren_close";
    case token::token_kind::angle_bracket_open:
        return "angle_bracket_open";
    case token::token_kind::angle_bracket_close:
        return "angle_bracket_close";
    default:
        return std::string("'") + static_cast<char>(kind) + "'";
    }
}

bool operator==(const token& lhs, const token& rhs)
{
    return std::tie(lhs.kind, lhs.token, lhs.line, lhs.column) ==
        std::tie(rhs.kind, rhs.token, rhs.line, rhs.column);
}

bool operator!=(const token& lhs, const token& rhs)
{
    return !(lhs == rhs);
}

}

}

}
