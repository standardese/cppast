#include <cppast/detail/parser/buffered_lexer.hpp>

using namespace cppast::detail::parser;

buffered_lexer::buffered_lexer(std::istream& input, std::size_t size) :
    lexer{input}
{
    _lookahead_buffer.reserve(size + 1);
    while(_lookahead_buffer.size() < _lookahead_buffer.capacity() && lexer::read_next_token())
    {
        _lookahead_buffer.push_back(lexer::current_token());
    }
}

const token& buffered_lexer::current_token() const
{
    return _current_token;
}

bool buffered_lexer::read_next_token()
{
    bool got_something = false;

    if(!lexer::good())
    {
        return false;
    }

    while(_lookahead_buffer.size() < _lookahead_buffer.capacity() && lexer::read_next_token())
    {
        _lookahead_buffer.push_back(lexer::current_token());
        got_something = true;
    }

    if(!_lookahead_buffer.empty())
    {
        _current_token = _lookahead_buffer.front();
        _lookahead_buffer.erase(_lookahead_buffer.begin());
        got_something = true;
    }

    return lexer::good() && got_something;
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
