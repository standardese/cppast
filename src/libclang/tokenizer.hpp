// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_TOKENIZER_HPP_INCLUDED
#define CPPAST_TOKENIZER_HPP_INCLUDED

#include <vector>

#include "raii_wrapper.hpp"

namespace cppast
{
    namespace detail
    {
        class token
        {
        public:
            explicit token(const CXTranslationUnit& tu_unit, const CXToken& token);

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

        inline bool operator==(const token& tok, const char* str) noexcept
        {
            return tok.value() == str;
        }

        inline bool operator==(const char* str, const token& tok) noexcept
        {
            return str == tok.value();
        }

        inline bool operator!=(const token& tok, const char* str) noexcept
        {
            return !(tok == str);
        }

        inline bool operator!=(const char* str, const token& tok) noexcept
        {
            return !(str == tok);
        }

        using token_iterator = std::vector<token>::const_iterator;

        class tokenizer
        {
        public:
            explicit tokenizer(const CXTranslationUnit& tu, const CXFile& file,
                               const CXCursor& cur);

            token_iterator begin() const noexcept
            {
                return tokens_.begin();
            }

            token_iterator end() const noexcept
            {
                return tokens_.end();
            }

            // if it returns true, the last token is ">>",
            // but should haven been ">"
            // only a problem for template parameters
            bool unmunch() const noexcept
            {
                return unmunch_;
            }

        private:
            std::vector<token> tokens_;
            bool               unmunch_;
        };

        class token_stream
        {
        public:
            explicit token_stream(const tokenizer& tokenizer, const CXCursor& cur)
            : cursor_(cur),
              begin_(tokenizer.begin()),
              cur_(begin_),
              end_(tokenizer.end()),
              unmunch_(tokenizer.unmunch())
            {
            }

            const token& peek() const noexcept
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

            const token& get() noexcept
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

            token_iterator begin() const noexcept
            {
                return begin_;
            }

            token_iterator cur() const noexcept
            {
                return cur_;
            }

            token_iterator end() const noexcept
            {
                return end_;
            }

            void set_cur(token_iterator iter) noexcept
            {
                cur_ = iter;
            }

            bool unmunch() const noexcept
            {
                return unmunch_;
            }

        private:
            CXCursor       cursor_;
            token_iterator begin_, cur_, end_;
            bool           unmunch_;
        };

        // skips the next token
        // asserts that it has the given string
        void skip(token_stream& stream, const char* str);

        // skips the next token if it has the given string
        // if multi_token == true, str can consist of multiple tokens optionally separated by whitespace
        bool skip_if(token_stream& stream, const char* str, bool multi_token = false);

        // returns the location of the closing bracket
        // the current token must be (,[,{ or <
        // note: < might not work in the arguments of a template specialization
        token_iterator find_closing_bracket(token_stream stream);

        // skips brackets
        // the current token must be (,[,{ or <
        // note: < might not work in the arguments of a template specialization
        void skip_brackets(token_stream& stream);

        // skips an attribute
        bool skip_attribute(token_stream& stream);
    }
} // namespace cppast::detail

#endif // CPPAST_TOKENIZER_HPP_INCLUDED
