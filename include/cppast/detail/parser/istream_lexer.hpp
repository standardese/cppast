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
    istream_lexer(std::istream& input, const diagnostic_logger& logger, const std::string& filename = "");

    bool read_next_token() override;
    const token& current_token() const override;

    bool good() const override;
    bool eof() const override;
    source_location location() const override;

private:
    std::istream& _input;
    std::string _filename;
    std::ostringstream _token_buffer;
    token _current_token;
    std::size_t _line, _column, _previous_line_length;
    char _last_char;

    void save_token(const token::token_kind kind, const std::string& token = "");
    bool advance();
    bool put_back();
};

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_ISTREAM_LEXER_HPP_INCLUDED
