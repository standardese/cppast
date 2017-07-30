// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_ISTREAM_LEXER_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_ISTREAM_LEXER_HPP_INCLUDED

#include <istream>
#include <sstream>
#include <cppast/detail/parser/lexer.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

class istream_lexer : public lexer
{
public:
    istream_lexer(std::istream& input, const diagnostic_logger& logger);

    bool read_next_token() override;
    const token& current_token() const override;

    bool good() const override;
    bool eof() const override;

private:
    std::istream& _input;
    std::ostringstream _token_buffer;
    token _current_token;
    std::size_t _pos;
    char _last_char;

    void save_token(const token::token_kind kind, const std::string& token = "");
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_ISTREAM_LEXER_HPP_INCLUDED