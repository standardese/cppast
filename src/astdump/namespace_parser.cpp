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
    // Nested namespaces are not detected.
    auto                   nested = false;
    cpp_namespace::builder builder(std::string{name}, inline_, nested);

    auto children = entity["inner"];
    if (children.error() != simdjson::error_code::NO_SUCH_FIELD)
    {
        for (dom::object child : children.value())
        {
            auto e = astdump_detail::parse_entity(context, builder.get(), child);
            if (e)
                builder.add_child(std::move(e));
        }
    }

    return builder.finish(*context.idx, id);
}

