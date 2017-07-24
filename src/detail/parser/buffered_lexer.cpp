// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/buffered_lexer.hpp>

using namespace cppast::detail::parser;

buffered_lexer::buffered_lexer(lexer& lexer, std::size_t size) :
    cppast::detail::parser::lexer{lexer.logger()},
    _lexer(lexer)
{
    _lookahead_buffer.reserve(size);
    reset();
}

void buffered_lexer::reset()
{
    while(_lookahead_buffer.size() < _lookahead_buffer.capacity() && _lexer.read_next_token())
    {
        _lookahead_buffer.push_back(_lexer.current_token());
    }
}

const token& buffered_lexer::current_token() const
{
    return _current_token;
}

bool buffered_lexer::read_next_token()
{
    bool got_something = false;

    while(_lookahead_buffer.size() < _lookahead_buffer.capacity() && _lexer.read_next_token())
    {
        _lookahead_buffer.push_back(_lexer.current_token());
        got_something = true;
    }

    if(!_lookahead_buffer.empty())
    {
        _current_token = _lookahead_buffer.front();
        _lookahead_buffer.erase(_lookahead_buffer.begin());
        got_something = true;
    }

    if(got_something)
    {
        logger().log("buffered_lexer.read_next_token", severity::debug, source_location::make_unknown(),
            "got: {} ", current_token());
    }

    return got_something;
}

std::size_t buffered_lexer::buffer_size() const
{
    return _lookahead_buffer.size();
}

const token& buffered_lexer::next_token(std::size_t i) const
{
    i = std::min(buffer_size() - 1, i);

    return _lookahead_buffer[i];
}

bool buffered_lexer::good() const
{
    return _lexer.good() && eof();
}

bool buffered_lexer::eof() const
{
    return _lookahead_buffer.empty();
}
