// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSE_ERROR_HPP_INCLUDED
#define CPPAST_PARSE_ERROR_HPP_INCLUDED

#include <stdexcept>

#include <cppast/diagnostic.hpp>
#include <debug_assert.hpp>

#include "debug_helper.hpp"

namespace cppast
{
namespace detail
{
    inline source_location make_location(const CXCursor& cur)
    {
        auto loc = clang_getCursorLocation(cur);

        CXString file;
        unsigned line;
        clang_getPresumedLocation(loc, &file, &line, nullptr);

        return source_location::make_file(cxstring(file).c_str(), line);
    }

    inline source_location make_location(const CXFile& file, const CXCursor& cur)
    {
        return source_location::make_entity(get_display_name(cur).c_str(),
                                            cxstring(clang_getFileName(file)).c_str());
    }

    inline source_location make_location(const CXType& type)
    {
        return source_location::make_entity(cxstring(clang_getTypeSpelling(type)).c_str());
    }

    // thrown on a parsing error
    // not meant to escape to the user
    class parse_error : public std::logic_error
    {
    public:
        parse_error(source_location loc, std::string message)
        : std::logic_error(std::move(message)), location_(std::move(loc))
        {}

        parse_error(const CXCursor& cur, std::string message)
        : parse_error(make_location(cur), std::move(message))
        {}

        parse_error(const CXType& type, std::string message)
        : parse_error(make_location(type), std::move(message))
        {}

        diagnostic get_diagnostic(const CXFile& file)
        {
            return get_diagnostic(cxstring(clang_getFileName(file)).c_str());
        }

        diagnostic get_diagnostic(std::string file)
        {
            location_.file = std::move(file);
            return diagnostic{what(), location_, severity::error};
        }

    private:
        source_location location_;
    };

    // DEBUG_ASSERT handler for parse errors
    // throws a parse_error exception
    struct parse_error_handler : debug_assert::set_level<1>, debug_assert::allow_exception
    {
        static void handle(const debug_assert::source_location&, const char*, const CXCursor& cur,
                           std::string message)
        {
            throw parse_error(cur, std::move(message));
        }

        static void handle(const debug_assert::source_location&, const char*, const CXType& type,
                           std::string message)
        {
            throw parse_error(type, std::move(message));
        }
    };
} // namespace detail
} // namespace cppast

#endif // CPPAST_PARSE_ERROR_HPP_INCLUDED
