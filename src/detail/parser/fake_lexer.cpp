// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/fake_lexer.hpp>
#include <cppast/detail/assert.hpp>

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
    DEBUG_ASSERT(_current_token >= 0 && static_cast<std::size_t>(_current_token) < _tokens.size(),
        detail::assert_handler());
    return _tokens[static_cast<std::size_t>(_current_token)];
}

source_location fake_lexer::location() const
{
    if(_current_token >= 0 && ! eof())
    {
        return source_location::make_file(
            "fake_lexer.cpp",
            static_cast<unsigned>(current_token().line),
            static_cast<unsigned>(current_token().column)
        );
    }
    else
    {
        return source_location::make_unknown();
    }
}

const std::vector<token>& fake_lexer::tokens() const
{
    return _tokens;
}
