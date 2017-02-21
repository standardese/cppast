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
            explicit token(const cxtranslation_unit& tu_unit, const CXToken& token);

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

        class tokenizer
        {
        public:
            explicit tokenizer(const cxtranslation_unit& tu, const CXFile& file,
                               const CXCursor& cur);

            std::vector<token>::const_iterator begin() const noexcept
            {
                return tokens_.begin();
            }

            std::vector<token>::const_iterator end() const noexcept
            {
                return tokens_.end();
            }

        private:
            std::vector<token> tokens_;
        };
    }
} // namespace cppast::detail

#endif // CPPAST_TOKENIZER_HPP_INCLUDED
