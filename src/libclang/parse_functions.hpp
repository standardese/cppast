// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
#define CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/parser.hpp>

#include "raii_wrapper.hpp"
#include "tokenizer.hpp"   // for convenience
#include "parse_error.hpp" // for convenience

namespace cppast
{
    class cpp_expression;
    class cpp_type;

    namespace detail
    {
        cpp_entity_id get_entity_id(const CXCursor& cur);

        struct parse_context
        {
            CXTranslationUnit                              tu;
            CXFile                                         file;
            type_safe::object_ref<const diagnostic_logger> logger;
            type_safe::object_ref<const cpp_entity_index>  idx;
        };

        std::unique_ptr<cpp_type> parse_type(const parse_context& context, const CXType& type);

        std::unique_ptr<cpp_expression> parse_expression(const parse_context& context,
                                                         const CXCursor&      cur);

        // parse_entity() dispatches on the cursor type
        // it calls one of the other parse functions defined elsewhere
        // try_parse_XXX are not exposed entities
        // they are called on an unexposed cursor and see whether they match

        std::unique_ptr<cpp_entity> try_parse_cpp_language_linkage(const parse_context& context,
                                                                   const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_cpp_namespace(const parse_context& context,
                                                        const CXCursor&      cur);
        std::unique_ptr<cpp_entity> parse_cpp_namespace_alias(const parse_context& context,
                                                              const CXCursor&      cur);
        std::unique_ptr<cpp_entity> parse_cpp_using_directive(const parse_context& context,
                                                              const CXCursor&      cur);
        std::unique_ptr<cpp_entity> parse_cpp_using_declaration(const parse_context& context,
                                                                const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_cpp_type_alias(const parse_context& context,
                                                         const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_cpp_enum(const parse_context& context,
                                                   const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_entity(const parse_context& context, const CXCursor& cur);
    }
} // namespace cppast::detail

#endif // CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
