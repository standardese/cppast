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

/// Contains information about a single token
///
/// \notes A token stores the original complete
/// lexed string plus a kind tag telling the cathegory
/// of the token (A literal, an operator, etc). Tokens
/// representing literals have an assoticated value
/// that can be extracted from the `string_value()`,
/// `int_value()`, `float_value()`, and `bool_value()`
/// functions. Invoking the wrong `xxxx_value()` function
/// has undefined behavior. Also invoking `xxxx_value()` functions
/// on non-literal tokens has UB too.
/// The token also stores information about the location
/// of the token in the original input (line, column, length).
struct token
{
    /// The token kind represents the meaning
    /// or purpose of a token
    ///
    /// \notes Tokens with specific kinds (Such as keywords, literals,
    /// etc) have an explicit kind in the enumeration labeled
    /// from -1 to -N. Raw input characters with no specific
    /// meaning (Such as 'a' or the plus operator '+') have their
    /// ascii code (positive char) as kind.
    /// The first value in the enumeration, `token_kind::unknown`, is
    /// an error placeholder value and has no corresponding meaning in
    /// the language.
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

    token_kind kind; /// Token cathegory
    std::string token; /// Original lexed string
    std::size_t line; /// Line where the token is located in the input (0 to N-1)
    std::size_t column; /// Column where the token is located in the input line (0 to N-1)

    /// Returns the length of the lexed string in the input
    ///
    /// \notes In combination with line and column, this function
    /// can be used to compute the source range the token takes in
    /// the input. Note tokens always belong to one line only.
    std::size_t length() const;

    /// Returns the value of a string literal token
    ///
    /// \notes An string literal token includes the surrounding
    /// string quotes (""), but the string value has the quotes
    /// stripped. For example, given a token "\"foo\"", the token
    /// string is "\"foo\"" and its string value is "foo"
    std::string string_value() const;

    /// Returns the value of an integer literal token
    long long int_value() const;

    /// Returns the value of an unsigned integer literal token
    unsigned long long uint_value() const;

    /// Returns the value of a floating-point literal token
    double float_value() const;

    /// Returns the value of a boolean literal token
    bool bool_value() const;
};

std::ostream& operator<<(std::ostream& os, const token::token_kind kind);
std::ostream& operator<<(std::ostream& os, const token& token);
bool operator==(const token& lhs, const token& rhs);
bool operator!=(const token& lhs, const token& rhs);

/// Returns a human readable string representing a given token
std::string to_string(const token& token);

/// Returns a human readable string corresponding to a given token kind
std::string to_string(token::token_kind kind);

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_TOKEN_HPP_INCLUDED
