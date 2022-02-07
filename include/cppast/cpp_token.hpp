// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_TOKEN_HPP_INCLUDED
#define CPPAST_CPP_TOKEN_HPP_INCLUDED

#include <string>
#include <vector>

#include <type_safe/reference.hpp>

#include <cppast/cppast_fwd.hpp>

namespace cppast
{
/// The kinds of C++ tokens.
enum class cpp_token_kind
{
    identifier,     //< Any identifier.
    keyword,        //< Any keyword.
    int_literal,    //< An integer literal.
    float_literal,  //< A floating point literal.
    char_literal,   //< A character literal.
    string_literal, //< A string literal.
    punctuation     //< Any other punctuation.
};

/// A C++ token.
struct cpp_token
{
    std::string    spelling;
    cpp_token_kind kind;

    cpp_token(cpp_token_kind kind, std::string spelling) : spelling(std::move(spelling)), kind(kind)
    {}

    friend bool operator==(const cpp_token& lhs, const cpp_token& rhs) noexcept
    {
        return lhs.spelling == rhs.spelling;
    }

    friend bool operator!=(const cpp_token& lhs, const cpp_token& rhs) noexcept
    {
        return !(rhs == lhs);
    }
};

/// A combination of multiple C++ tokens.
class cpp_token_string
{
public:
    /// Builds a token string.
    class builder
    {
    public:
        builder() = default;

        /// \effects Adds a token.
        void add_token(cpp_token tok)
        {
            tokens_.push_back(std::move(tok));
        }

        /// \effects Converts a trailing `>>` to `>` token.
        void unmunch();

        /// \returns The finished string.
        cpp_token_string finish()
        {
            return cpp_token_string(std::move(tokens_));
        }

    private:
        std::vector<cpp_token> tokens_;
    };

    /// Tokenizes a string.
    /// \effects Splits the string into C++ tokens.
    /// The string must contain valid tokens and must already be preprocessed (i.e. translation
    /// phase 6 is already done). \returns The tokenized string.
    static cpp_token_string tokenize(std::string str);

    /// \effects Creates it from a sequence of tokens.
    cpp_token_string(std::vector<cpp_token> tokens) : tokens_(std::move(tokens)) {}

    /// \exclude target
    using iterator = std::vector<cpp_token>::const_iterator;

    /// \returns An iterator to the first token.
    iterator begin() const noexcept
    {
        return tokens_.begin();
    }

    /// \returns An iterator one past the last token.
    iterator end() const noexcept
    {
        return tokens_.end();
    }

    /// \returns Whether or not the string is empty.
    bool empty() const noexcept
    {
        return tokens_.empty();
    }

    /// \returns A reference to the first token.
    const cpp_token& front() const noexcept
    {
        return tokens_.front();
    }

    /// \returns A reference to the last token.
    const cpp_token& back() const noexcept
    {
        return tokens_.back();
    }

    /// \returns The string representation of the tokens, without any whitespace.
    std::string as_string() const;

private:
    std::vector<cpp_token> tokens_;

    friend bool operator==(const cpp_token_string& lhs, const cpp_token_string& rhs);
};

bool operator==(const cpp_token_string& lhs, const cpp_token_string& rhs);

inline bool operator!=(const cpp_token_string& lhs, const cpp_token_string& rhs)
{
    return !(lhs == rhs);
}
} // namespace cppast

#endif // CPPAST_CPP_TOKEN_HPP_INCLUDED
