// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_namespace.hpp>

using namespace cppast;

std::unique_ptr<cpp_entity> astdump_detail::parse_namespace(parse_context& context,
                                                            dom::object    entity)
{
    auto id = get_entity_id(context, entity);

    auto name = [&] {
        auto field = entity["name"].get_string();
        if (field.error() != simdjson::error_code::NO_SUCH_FIELD)
            return field.value();
        else
            return std::string_view("");
    }();
    auto inline_ = [&] {
        auto field = entity["isInline"].get_bool();
        if (field.error() != simdjson::error_code::NO_SUCH_FIELD)
            return field.value();
        else
            return false;
    }();
    // TODO: Nested namespaces are not detected.
    auto                   nested = false;
    cpp_namespace::builder builder(std::string{name}, inline_, nested);

    auto children = entity["inner"];
    if (children.error() != simdjson::error_code::NO_SUCH_FIELD)
        for (dom::object child : children.value())
        {
            auto e = astdump_detail::parse_entity(context, builder.get(), child);
            if (e)
                builder.add_child(std::move(e));
        }

    return builder.finish(*context.idx, id);
}

std::unique_ptr<cpp_entity> astdump_detail::parse_namespace_alias(parse_context& context,
                                                                  dom::object    entity)
{
    auto id = get_entity_id(context, entity);

    auto name = entity["name"].get_string().value();

    auto target    = entity["aliasedNamespace"].get_object().value();
    auto target_id = get_entity_id(context, target);
    // TODO: target name is a simplification and not the exact spelling as in the source.
    auto target_name = target["name"].get_string().value();

    auto result
        = cpp_namespace_alias::build(*context.idx, id, std::string(name),
                                     cpp_namespace_ref(target_id, std::string(target_name)));
    handle_comment_child(context, *result, entity);
    return result;
}

std::unique_ptr<cpp_entity> astdump_detail::parse_using_directive(parse_context& context,
                                                                  dom::object    entity)
{
    auto target    = entity["nominatedNamespace"].get_object().value();
    auto target_id = get_entity_id(context, target);
    // TODO: target name is a simplification and not the exact spelling as in the source.
    auto target_name = target["name"].get_string().value();

    auto result
        = cpp_using_directive::build(cpp_namespace_ref(target_id, std::string(target_name)));
    handle_comment_child(context, *result, entity);
    return result;
}

