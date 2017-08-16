// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_BUFFERED_LEXER_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_BUFFERED_LEXER_HPP_INCLUDED

#include <cppast/detail/parser/lexer.hpp>
#include <vector>

namespace cppast
{

namespace detail
{

namespace parser
{

/// Implements a lexer with token lookahead features
///
/// `buffered_lexer` implements a lookahead layer on top
/// of an existing lexer, so LL(k) parsers can be implemented
/// easily. The buffered lexer takes an existing lexer and the
/// maximum number of lookahead tokens required (k). The `lexer`
/// interface contract is followed (`read_next_token()` reads the next
/// token in the stream and puts it in `current_token()` for read)
/// but also takes care of reading and storing "future" tokens so these
/// can be queried in advance.
///
/// The location and state of the underlying lexer is hidden and completely
/// undefined, also manipulating the underlying lexer while being used by a
/// buffered lexer has undefined behavior.
/// The exact moment when the underlying lexer is manipulated is undefined,
/// the only property that holds is that if there are more than K tokens left
/// to scan in the input (from the buffered lexer `current_token()` point
/// of view) the buffered lexer will always expose K lookahead tokens.
///
/// Note that it could happen that no lookahead tokens are available at some
/// point, such as when reaching the end of the input. To check how much we
/// could query in the future the class exposes `buffer_size()` function to
/// tell how many lookahead tokens to query are available (Never more than K).
class buffered_lexer : public lexer
{
public:
    /// Initializes a buffered lexer given a lexer and the maximum number
    /// of lookahead tokens required (K)
    buffered_lexer(lexer& lexer, std::size_t size);

    /// Reads the next token in the input and puts it available
    /// at `current_token()`.
    /// \returns True if a new token was successfully read, false otherwise.
    bool read_next_token() override;

    /// Returns the last token read from the input, the token the lexer is
    /// currently pointing to.
    const token& current_token() const override;

    /// Returns the source location in the input the lexer is pointing to.
    source_location location() const override;

    /// Returns the i-th next token in the input
    /// \returns For an index `i` in `[0, buffer_size())` returns
    /// the token `i` positions to the right from the current token.
    /// \notes Note that lexers scan left to right, so next means a
    /// token to the right of the current location.
    const token& next_token(std::size_t i) const;

    /// Returns the number of lookahead tokens available to read.
    std::size_t buffer_size() const;

    /// Checks whether the lexer can continue reading input.
    bool good() const override;

    /// Checks whether the lexer has reached the end of the input.
    bool eof() const override;

private:
    lexer& _lexer;
    token _current_token;
    std::vector<token> _lookahead_buffer;
};

}

}

}

#endif // CPPAST_DETAIL_PARSER_BUFFERED_LEXER_HPP_INCLUDED
