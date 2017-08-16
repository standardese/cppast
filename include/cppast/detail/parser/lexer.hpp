// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_LEXER_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_LEXER_HPP_INCLUDED

#include <cppast/detail/info_tracking_logger.hpp>
#include <cppast/detail/parser/token.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

/// Returns a token stream out of some parsing input
///
/// lexer provides an interface to tokenize some input and
/// return the resulting tokens. The interface works as a live
/// scanner where a new (the next) token from the input is returned
/// on demand, by invoking `read_next_token()` function. If `read_next_token()`
/// successfuly read a new token, returns true and puts the new token
/// accessible through `current_token()` function. When no more tokens
/// are available (Or reading the next token resulted in a lexing error)
/// `read_next_token()` returns false. Invoking `current_token()` before
/// invoking `read_next_token()` or after `read_next_token()` returned false
/// has undefined behavior.
///
/// In general, the usage of `current_token()` and `read_next_token()` after
/// a `false` return from `read_next_token()` has undefined behavior.
///
/// The interface is designed with a "fetch tokens loop" in mind:
///
/// ``` cpp
/// while(lexer.read_next_token())
/// {
///     do_something(lexer.current_token());
/// }
///
/// See `lexer_example.cpp` for a full example.
class lexer
{
public:
    virtual ~lexer() = default;

    /// Scans the next token from the input
    /// \returns true if a new token was read from the input. False if
    /// an error occurred or if there was no more input available.
    /// \notes When returning true on a successful read, the read token
    /// becomes accessible through `current_token()`.
    virtual bool read_next_token() = 0;

    /// Returns the last token read by the lexer, if any
    virtual const token& current_token() const = 0;

    /// Checks if the lexer can continue reading tokens
    /// \returns true if the lexer can continue scanning, false otherwise
    virtual bool good() const = 0;

    /// Checks if the lexer reached the end of input
    virtual bool eof() const = 0;

    /// Returns the source location in the input the lexer is currently
    /// pointing to.
    /// \notes It's equivalent to the source location of the current token.
    virtual source_location location() const = 0;

    /// Returns the diagnostic logger being used by the lexe.
    const diagnostic_logger& logger() const;

protected:
    lexer(const diagnostic_logger& logger);

    cppast::detail::info_tracking_logger _logger;
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_LEXER_HPP_INCLUDED
