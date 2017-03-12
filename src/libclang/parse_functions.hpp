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
    enum cpp_storage_class_specifiers : int;

    namespace detail
    {
        cpp_entity_id get_entity_id(const CXCursor& cur);

        // only use this if the name is just a single token
        // never where it is a reference to something (like base class name)
        // as then you won't get it "as-is"
        cxstring get_cursor_name(const CXCursor& cur);

        // note: does not handle thread_local
        cpp_storage_class_specifiers get_storage_class(const CXCursor& cur);

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
        // parse the expression starting at the current token in the stream
        // and ends at the given iterator
        // this is required for situations where there is no expression cursor exposed,
        // like member initializers
        std::unique_ptr<cpp_expression> parse_raw_expression(const parse_context&      context,
                                                             token_stream&             stream,
                                                             token_iterator            end,
                                                             std::unique_ptr<cpp_type> type);

        // parse_entity() dispatches on the cursor type
        // it calls one of the other parse functions defined elsewhere
        // try_parse_XXX are not exposed/differently exposed entities
        // they are called on corresponding cursor and see whether they match

        // unexposed
        std::unique_ptr<cpp_entity> try_parse_cpp_language_linkage(const parse_context& context,
                                                                   const CXCursor&      cur);
        // CXXMethod
        std::unique_ptr<cpp_entity> try_parse_static_cpp_function(const parse_context& context,
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
        std::unique_ptr<cpp_entity> parse_cpp_class(const parse_context& context,
                                                    const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_cpp_variable(const parse_context& context,
                                                       const CXCursor&      cur);
        // also parses bitfields
        std::unique_ptr<cpp_entity> parse_cpp_member_variable(const parse_context& context,
                                                              const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_cpp_function(const parse_context& context,
                                                       const CXCursor&      cur);

        std::unique_ptr<cpp_entity> parse_entity(const parse_context& context, const CXCursor& cur);
    }
} // namespace cppast::detail

#endif // CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
