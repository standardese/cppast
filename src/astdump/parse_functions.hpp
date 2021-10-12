// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED
#define ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/parser.hpp>
#include <fstream>
#include <simdjson.h>

namespace cppast
{
class cpp_using_declaration;

namespace astdump_detail
{
    namespace dom = simdjson::ondemand; // Technically lying, as it's not simdjson::dom.

    source_location get_location(dom::object& entity);

    struct parse_context
    {
        std::string                                    path;
        std::ifstream                                  file;
        type_safe::object_ref<const diagnostic_logger> logger;
        type_safe::object_ref<const cpp_entity_index>  idx;

        type_safe::optional_ref<cpp_using_declaration> last_using_decl;

        bool error = false;

        explicit parse_context(type_safe::object_ref<const diagnostic_logger> logger,
                               type_safe::object_ref<const cpp_entity_index> idx, std::string path)
        : path(std::move(path)), file(path), logger(logger), idx(idx)
        {}
    };

    cpp_entity_id get_entity_id(parse_context& context, dom::object& entity);

    std::string parse_comment(parse_context& context, dom::object entity);
    void handle_comment_child(parse_context& context, cpp_entity& entity, dom::object object);

    std::unique_ptr<cpp_expression> parse_expression(parse_context& context, dom::value type);
    std::unique_ptr<cpp_type>       parse_type(parse_context& context, dom::value type);

    std::unique_ptr<cpp_entity> parse_unexposed_entity(parse_context& context, dom::object entity);
    std::unique_ptr<cpp_entity> parse_language_linkage(parse_context& context, dom::object entity);
    std::unique_ptr<cpp_entity> parse_namespace(parse_context& context, dom::object entity);
    std::unique_ptr<cpp_entity> parse_namespace_alias(parse_context& context, dom::object entity);
    std::unique_ptr<cpp_entity> parse_using_directive(parse_context& context, dom::object entity);
    std::unique_ptr<cpp_entity> parse_type_alias(parse_context& context, dom::object entity);

    std::unique_ptr<cpp_entity> parse_using_declaration(parse_context& context, dom::object entity);
    void parse_shadow_using_declaration(parse_context& context, dom::object entity);

    std::unique_ptr<cpp_entity> parse_entity(parse_context& context, cpp_entity& parent,
                                             std::string_view kind, dom::object entity);

    inline std::unique_ptr<cpp_entity> parse_entity(parse_context& context, cpp_entity& parent,
                                                    dom::object entity)
    {
        auto kind = entity["kind"].get_string().value();
        return parse_entity(context, parent, kind, entity);
    }
} // namespace astdump_detail
} // namespace cppast

#endif // ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED
