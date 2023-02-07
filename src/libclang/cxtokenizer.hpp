// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CXTOKENIZER_HPP_INCLUDED
#define CPPAST_CXTOKENIZER_HPP_INCLUDED

#include <string>
#include <vector>

#include <cppast/cpp_attribute.hpp>
#include <cppast/cpp_token.hpp>

#include "raii_wrapper.hpp"

namespace cppast
{
namespace detail
{
    class cxtoken
    {
    public:
        explicit cxtoken(const CXTranslationUnit& tu_unit, const CXToken& token);

        const cxstring& value() const noexcept
        {
            return value_;
        }

        const char* c_str() const noexcept
        {
            return value_.c_str();
        }

        CXTokenKind kind() const noexcept
        {
            return kind_;
        }

    private:
        cxstring    value_;
        CXTokenKind kind_;
    };

    inline bool operator==(const cxtoken& tok, const char* str) noexcept
    {
        return tok.value() == str;
    }

    inline bool operator==(const char* str, const cxtoken& tok) noexcept
    {
        return str == tok.value();
    }

    inline bool operator!=(const cxtoken& tok, const char* str) noexcept
    {
        return !(tok == str);
    }

    inline bool operator!=(const char* str, const cxtoken& tok) noexcept
    {
        return !(str == tok);
    }

    using cxtoken_iterator = std::vector<cxtoken>::const_iterator;

    class cxtokenizer
    {
    public:
        explicit cxtokenizer(const CXTranslationUnit& tu, const CXFile& file, const CXCursor& cur);

        cxtoken_iterator begin() const noexcept
        {
            return tokens_.begin();
        }

        cxtoken_iterator end() const noexcept
        {
            return tokens_.end();
        }

    private:
        std::vector<cxtoken> tokens_;
    };

    class cxtoken_stream
    {
    public:
        explicit cxtoken_stream(const cxtokenizer& tokenizer, const CXCursor& cur)
        : cursor_(cur), begin_(tokenizer.begin()), cur_(begin_), end_(tokenizer.end())
        {}

        const cxtoken& peek() const noexcept
        {
            if (done())
                return *std::prev(end_);
            return *cur_;
        }

        void bump() noexcept
        {
            if (cur_ != end_)
                ++cur_;
        }

        void bump_back() noexcept
        {
            if (cur_ != begin_)
                --cur_;
        }

        const cxtoken& get() noexcept
        {
            auto& result = peek();
            bump();
            return result;
        }

        bool done() const noexcept
        {
            return cur_ == end_;
        }

        const CXCursor& cursor() const noexcept
        {
            return cursor_;
        }

        cxtoken_iterator begin() const noexcept
        {
            return begin_;
        }

        cxtoken_iterator cur() const noexcept
        {
            return cur_;
        }

        cxtoken_iterator end() const noexcept
        {
            return end_;
        }

        void set_cur(cxtoken_iterator iter) noexcept
        {
            cur_ = iter;
        }

    private:
        CXCursor         cursor_;
        cxtoken_iterator begin_, cur_, end_;
    };

    // skips the next token
    // asserts that it has the given string
    void skip(cxtoken_stream& stream, const char* str);

    // skips the next token if it has the given string
    // if multi_token == true, str can consist of multiple tokens optionally separated by whitespace
    bool skip_if(cxtoken_stream& stream, const char* str, bool multi_token = false);

    struct closing_bracket_pos
    {
        // If unmunch == false: bracket points to the closing bracket, after is the iterator after
        // that. If unmunch == true: bracket points to >>, after points to the same >>; only one
        // bracket is part of the matching closing one.
        cxtoken_iterator bracket, after;
        bool             unmunch;
    };

    // returns the location of the closing bracket
    // the current token must be (,[,{ or <
    // note: < might not work in the arguments of a template specialization
    closing_bracket_pos find_closing_bracket(cxtoken_stream stream);

    // skips brackets
    // the current token must be (,[,{ or <
    // note: < might not work in the arguments of a template specialization
    void skip_brackets(cxtoken_stream& stream);

    // finds the location of the given sequence in a stream
    // returns an iterator to the first token of the found sequence, or stream.end() if not found
    cxtoken_iterator find_sequence(cxtoken_stream stream, cxtoken_iterator start,
                                   cxtoken_iterator end);

    // parses attributes
    // if skip_anyway is true it will bump even if no attributes have been parsed
    cpp_attribute_list parse_attributes(cxtoken_stream& stream, bool skip_anyway = false);

    // converts a token range to a string
    cpp_token_string to_string(cxtoken_stream& stream, cxtoken_iterator end, bool unmunch);

    // appends token to scope, if it is still valid
    // else clears it
    // note: does not consume the token if it is not valid,
    // returns false in that case
    bool append_scope(cxtoken_stream& stream, std::string& scope);
} // namespace detail
} // namespace cppast

#endif // CPPAST_CXTOKENIZER_HPP_INCLUDED
