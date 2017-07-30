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

class lexer
{
public:
    virtual ~lexer() = default;

    virtual bool read_next_token() = 0;
    virtual const token& current_token() const = 0;
    virtual bool good() const = 0;
    virtual bool eof() const = 0;

    const diagnostic_logger& logger() const;

protected:
    lexer(const diagnostic_logger& logger);

    cppast::detail::info_tracking_logger _logger;
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_LEXER_HPP_INCLUDED
