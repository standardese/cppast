// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_token.hpp>

#include <algorithm>
#include <cctype>
#include <cppast/detail/assert.hpp>

using namespace cppast;

void cpp_token_string::builder::unmunch()
{
    DEBUG_ASSERT(!tokens_.empty() && tokens_.back().spelling == ">>", detail::assert_handler{});
    tokens_.back().spelling = ">";
}

namespace
{
    bool is_identifier(char c)
    {
        return std::isalnum(c) || c == '_';
    }
}

std::string cpp_token_string::as_string() const
{
    std::string result;
    for (auto& token : tokens_)
    {
        DEBUG_ASSERT(!token.spelling.empty(), detail::assert_handler{});
        if (!result.empty() && is_identifier(result.back()) && is_identifier(token.spelling[0u]))
            result += ' ';
        result += token.spelling;
    }
    return result;
}

bool cppast::operator==(const cpp_token_string& lhs, const cpp_token_string& rhs)
{
    if (lhs.tokens_.size() != rhs.tokens_.size())
        return false;
    return std::equal(lhs.tokens_.begin(), lhs.tokens_.end(), rhs.tokens_.begin());
}
