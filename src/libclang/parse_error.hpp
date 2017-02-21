// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSE_ERROR_HPP_INCLUDED
#define CPPAST_PARSE_ERROR_HPP_INCLUDED

#include <stdexcept>

#include <debug_assert.hpp>
#include <cppast/diagnostic.hpp>

#include "debug_helper.hpp"

namespace cppast
{
    namespace detail
    {
        // thrown on a parsing error
        // not meant to escape to the user
        class parse_error : public std::logic_error
        {
        public:
            parse_error(source_location loc, std::string message)
            : std::logic_error(std::move(message)), location_(std::move(loc))
            {
            }

            parse_error(const CXCursor& cur, std::string message)
            : parse_error(source_location::make(get_display_name(cur).c_str()), std::move(message))
            {
            }

            diagnostic get_diagnostic() const
            {
                return diagnostic{what(), location_, severity::error};
            }

        private:
            source_location location_;
        };

        // DEBUG_ASSERT handler for parse errors
        // throws a parse_error exception
        struct parse_error_handler : debug_assert::set_level<1>, debug_assert::allow_exception
        {
            static void handle(const debug_assert::source_location&, const char*,
                               const CXCursor& cur, const char* message)
            {
                throw parse_error(cur, message);
            }
        };
    }
} // namespace cppast::detail

#endif // CPPAST_PARSE_ERROR_HPP_INCLUDED
