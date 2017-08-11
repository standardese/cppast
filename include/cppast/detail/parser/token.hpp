// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_TOKEN_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_TOKEN_HPP_INCLUDED

#include <cstdint>
#include <string>
#include <ostream>

namespace cppast
{

namespace detail
{

namespace parser
{

struct token
{
    enum class token_kind : char
    {
        unknown              = -1,
        identifier           = -2,
        string_literal       = -3,
        int_literal          = -4,
        float_literal        = -5,
        bool_literal         = -6,
        double_colon         = -7,
        comma                = -8,
        semicolon            = -9,
        bracket_open         = -10,
        bracket_close        = -11,
        double_bracket_open  = -12,
        double_bracket_close = -13,
        paren_open           = -14,
        paren_close          = -15,
        angle_bracket_open   = -16,
        angle_bracket_close  = -17,
        end_of_enum          = -18
    };

    token_kind kind;
    std::string token;
    std::size_t line;
    std::size_t column;

    std::size_t length() const;

    std::string string_value() const;
    long long int_value() const;
    unsigned long long uint_value() const;
    double float_value() const;
    bool bool_value() const;
};

std::ostream& operator<<(std::ostream& os, const token::token_kind kind);
std::ostream& operator<<(std::ostream& os, const token& token);
bool operator==(const token& lhs, const token& rhs);
bool operator!=(const token& lhs, const token& rhs);
std::string to_string(const token& token);
std::string to_string(token::token_kind kind);

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_TOKEN_HPP_INCLUDED
