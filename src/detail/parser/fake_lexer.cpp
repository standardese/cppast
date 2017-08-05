// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/fake_lexer.hpp>

using namespace cppast;
using namespace cppast::detail::parser;

fake_lexer::fake_lexer(const std::vector<token>& tokens, const diagnostic_logger& logger) :
    lexer{logger},
    _current_token{-1},
    _tokens{tokens}
{}

bool fake_lexer::eof() const
{
    return _current_token >= static_cast<std::int64_t>(_tokens.size());
}

bool fake_lexer::good() const
{
    return eof();
}

bool fake_lexer::read_next_token()
{
    if(_current_token < static_cast<std::int64_t>(_tokens.size()) - 1)
    {
        _current_token++;
        return true;
    }
    else
    {
        return false;
    }
}

const token& fake_lexer::current_token() const
{
    if(_current_token < 0 || eof())
    {
        logger().log("fake_lexer.current_token", severity::critical, source_location::make_unknown(),
            "Cannot read current token pointing at {} (total tokens: {})", _current_token, _tokens.size());

        throw std::runtime_error{"cannot read current token"};
    }

    return _tokens[_current_token];
}

source_location fake_lexer::location() const
{
    return source_location::make_file("fake_lexer.cpp", current_token().line);
}

const std::vector<token>& fake_lexer::tokens() const
{
    return _tokens;
}
