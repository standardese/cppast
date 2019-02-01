// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
#define CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/parser.hpp>

#include "cxtokenizer.hpp" // for convenience
#include "parse_error.hpp" // for convenience
#include "preprocessor.hpp"
#include "raii_wrapper.hpp"

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

    class comment_context
    {
    public:
        explicit comment_context(std::vector<pp_doc_comment>& comments)
        : cur_(comments.data()), end_(comments.data() + comments.size())
        {}

        // must be called for entities that want an associated comment
        // must be called *BEFORE* the children are added
        void match(cpp_entity& e, const CXCursor& cur) const;
        void match(cpp_entity& e, unsigned line, bool skip_comments = true) const;

    private:
        mutable pp_doc_comment* cur_;
        pp_doc_comment*         end_;
    };

    struct parse_context
    {
        CXTranslationUnit                              tu;
        CXFile                                         file;
        type_safe::object_ref<const diagnostic_logger> logger;
        type_safe::object_ref<const cpp_entity_index>  idx;
        comment_context                                comments;
        mutable bool                                   error;
    };

    // parse default value of variable, function parameter...
    std::unique_ptr<cpp_expression> parse_default_value(cpp_attribute_list&  attributes,
                                                        const parse_context& context,
                                                        const CXCursor& cur, const char* name);

    std::unique_ptr<cpp_type> parse_type(const parse_context& context, const CXCursor& cur,
                                         const CXType& type);

    // parse the type starting at the current token stream
    // and ends at the given iterator
    // this is required for situations where there is no type exposed,
    // like default type of a template type parameter
    std::unique_ptr<cpp_type> parse_raw_type(const parse_context& context, cxtoken_stream& stream,
                                             cxtoken_iterator end);

    std::unique_ptr<cpp_expression> parse_expression(const parse_context& context,
                                                     const CXCursor&      cur);
    // parse the expression starting at the current token in the stream
    // and ends at the given iterator
    // this is required for situations where there is no expression cursor exposed,
    // like member initializers
    std::unique_ptr<cpp_expression> parse_raw_expression(const parse_context&      context,
                                                         cxtoken_stream&           stream,
                                                         cxtoken_iterator          end,
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

    // on all function cursors except on destructor
    std::unique_ptr<cpp_entity> try_parse_cpp_function_template_specialization(
        const parse_context& context, const CXCursor& cur, bool is_friend);

    // on class cursors
    std::unique_ptr<cpp_entity> try_parse_full_cpp_class_template_specialization(
        const parse_context& context, const CXCursor& cur);

    std::unique_ptr<cpp_entity> parse_cpp_namespace(const parse_context& context,
                                                    cpp_entity& parent, const CXCursor& cur);
    std::unique_ptr<cpp_entity> parse_cpp_namespace_alias(const parse_context& context,
                                                          const CXCursor&      cur);
    std::unique_ptr<cpp_entity> parse_cpp_using_directive(const parse_context& context,
                                                          const CXCursor&      cur);
    std::unique_ptr<cpp_entity> parse_cpp_using_declaration(const parse_context& context,
                                                            const CXCursor&      cur);

    std::unique_ptr<cpp_entity> parse_cpp_type_alias(const parse_context& context,
                                                     const CXCursor&      cur,
                                                     const CXCursor&      template_cur);
    std::unique_ptr<cpp_entity> parse_cpp_enum(const parse_context& context, const CXCursor& cur);
    std::unique_ptr<cpp_entity> parse_cpp_class(const parse_context& context, const CXCursor& cur,
                                                const CXCursor& parent_cur);

    std::unique_ptr<cpp_entity> parse_cpp_variable(const parse_context& context,
                                                   const CXCursor&      cur);
    // also parses bitfields
    std::unique_ptr<cpp_entity> parse_cpp_member_variable(const parse_context& context,
                                                          const CXCursor&      cur);

    std::unique_ptr<cpp_entity> parse_cpp_function(const parse_context& context,
                                                   const CXCursor& cur, bool is_friend);
    std::unique_ptr<cpp_entity> parse_cpp_member_function(const parse_context& context,
                                                          const CXCursor& cur, bool is_friend);
    std::unique_ptr<cpp_entity> parse_cpp_conversion_op(const parse_context& context,
                                                        const CXCursor& cur, bool is_friend);
    std::unique_ptr<cpp_entity> parse_cpp_constructor(const parse_context& context,
                                                      const CXCursor& cur, bool is_friend);
    std::unique_ptr<cpp_entity> parse_cpp_destructor(const parse_context& context,
                                                     const CXCursor& cur, bool is_friend);

    std::unique_ptr<cpp_entity> parse_cpp_friend(const parse_context& context, const CXCursor& cur);

    std::unique_ptr<cpp_entity> parse_cpp_alias_template(const parse_context& context,
                                                         const CXCursor&      cur);
    std::unique_ptr<cpp_entity> parse_cpp_function_template(const parse_context& context,
                                                            const CXCursor& cur, bool is_friend);
    std::unique_ptr<cpp_entity> parse_cpp_class_template(const parse_context& context,
                                                         const CXCursor&      cur);
    std::unique_ptr<cpp_entity> parse_cpp_class_template_specialization(
        const parse_context& context, const CXCursor& cur);

    std::unique_ptr<cpp_entity> parse_cpp_static_assert(const parse_context& context,
                                                        const CXCursor&      cur);

    // parent: used for nested namespace, doesn't matter otherwise
    // parent_cur: used when parsing templates or friends
    std::unique_ptr<cpp_entity> parse_entity(const parse_context& context, cpp_entity* parent,
                                             const CXCursor& cur,
                                             const CXCursor& parent_cur = clang_getNullCursor());
} // namespace detail
} // namespace cppast

#endif // CPPAST_PARSE_FUNCTIONS_HPP_INCLUDED
