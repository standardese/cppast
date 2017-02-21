// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
#define CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/parser.hpp>

#include "raii_wrapper.hpp"

namespace cppast
{
    namespace detail
    {
        struct parse_context
        {
            CXTranslationUnit                              tu;
            CXFile                                         file;
            type_safe::object_ref<const diagnostic_logger> logger;
            type_safe::object_ref<const cpp_entity_index>  idx;
        };

        std::unique_ptr<cpp_entity> parse_entity(const parse_context& context, const CXCursor& cur);
    }
} // namespace cppast::detail

#endif // CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
