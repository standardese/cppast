// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_FAKE_LEXER_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_FAKE_LEXER_HPP_INCLUDED

#include <vector>
#include <cppast/detail/parser/lexer.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

/// Implements an interface to a sequence of already lexed tokens
///
/// Fake lexer provides a way to feed a parser with an existing sequence
/// of tokens, such as tokens taken directly from the C++ AST
class fake_lexer : public lexer
{
public:
    /// Initializes a fake lexer with the sequence of tokens to "read" and
    /// a diagnostic logger for the lexer.
    fake_lexer(const std::vector<token>& tokens, const diagnostic_logger& logger);

    /// Reads the next token from the secuence and puts it available to
    /// read at `current_token()`.
    /// \returns True if a token was read from the sequence, false if there
    /// are no more tokens in the sequence.
    bool read_next_token() override;

    /// Returns the token the sequence the lexer is currently pointing
    /// to. The behavior is undefined if the function is called when the lexer
    /// has already consumed the full sequence.
    const token& current_token() const override;

    /// Checks if the lexer can continue ready¡ing tokens.
    bool good() const override;

    /// Checks if the lexer has reached the end of the sequence.
    /// \notes Equivalent to `good()` ?
    bool eof() const override;

    /// Returns the source location of the token the lexer is currently
    /// pointing to.
    source_location location() const override;

    /// Returns the full sequence of tokens
    const std::vector<token>& tokens() const;

private:
    std::int64_t _current_token;
    std::vector<token> _tokens;
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_FAKE_LEXER_HPP_INCLUDED
