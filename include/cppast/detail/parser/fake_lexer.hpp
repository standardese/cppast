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

class fake_lexer : public lexer
{
public:
    fake_lexer(const std::vector<token>& tokens, const diagnostic_logger& logger);

    bool read_next_token() override;
    const token& current_token() const override;

    bool good() const override;
    bool eof() const override;

    source_location location() const override;

    const std::vector<token>& tokens() const;

private:
    std::int64_t _current_token;
    std::vector<token> _tokens;
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_FAKE_LEXER_HPP_INCLUDED
