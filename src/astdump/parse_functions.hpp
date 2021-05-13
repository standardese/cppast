// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED
#define ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/parser.hpp>
#include <fstream>
#include <simdjson.h>

namespace cppast
{
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
        bool                                           error = false;

        explicit parse_context(type_safe::object_ref<const diagnostic_logger> logger,
                               type_safe::object_ref<const cpp_entity_index> idx, std::string path)
        : path(std::move(path)), file(path), logger(logger), idx(idx)
        {}
    };

    cpp_entity_id get_entity_id(parse_context& context, dom::object& entity);

    std::unique_ptr<cpp_entity> parse_unexposed_entity(parse_context& context, dom::object& entity);

    std::unique_ptr<cpp_entity> parse_entity(parse_context& context, std::string_view kind,
                                             dom::object& entity);

    inline std::unique_ptr<cpp_entity> parse_entity(parse_context& context, dom::object& entity)
    {
        auto kind = entity["kind"].get_string().value();
        return parse_entity(context, kind, entity);
    }
} // namespace astdump_detail
} // namespace cppast

#endif // ASTDUMP_PARSE_FUNCTIONS_HPP_INCLUDED
