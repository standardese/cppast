// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_enum.hpp>

using namespace cppast;

namespace
{
std::unique_ptr<cpp_enum_value> parse_enum_value(astdump_detail::parse_context& context,
                                                 simdjson::ondemand::object     entity)
{
    auto id   = get_entity_id(context, entity);
    auto name = entity["name"].get_string().value();

    auto value = [&] {
        auto inner = entity["inner"].get_array();
        if (inner.error() == simdjson::NO_SUCH_FIELD)
            return std::unique_ptr<cpp_expression>(nullptr);
        else
            return astdump_detail::parse_expression(context, inner.at(0).value());
    }();

    return cpp_enum_value::build(*context.idx, id, std::string(name), std::move(value));
}
} // namespace

std::unique_ptr<cpp_entity> astdump_detail::parse_enum(parse_context& context, dom::object entity)
{
    auto id = get_entity_id(context, entity);

    auto is_declaration = [&] {
        auto loc_offset       = entity["loc"]["offset"].get_uint64().value();
        auto range_end_offset = entity["range"]["end"]["offset"].get_uint64().value();

        // For a forward declaration, the location and end are at the same offset (the name).
        return loc_offset == range_end_offset;
    }();

    auto name      = entity["name"].get_string().value();
    auto is_scoped = entity["scopedEnumTag"].get_string().error() != simdjson::NO_SUCH_FIELD;

    auto underlying_type = [&]() -> std::unique_ptr<cpp_type> {
        auto field = entity["fixedUnderlyingType"];
        if (field.error() != simdjson::NO_SUCH_FIELD)
            return parse_enum_underlying_type(context, field.value());
        else
            // This is technically only true for scoped enums.
            // Non-scoped enums can have a greater underlying types.
            return cpp_builtin_type::build(cpp_int);
    }();

    // Heuristic: if the type isn't int, the type is user-defined.
    // This breaks for types that explicitly indicate their underlying type is int.
    auto has_underlying_type
        = underlying_type->kind() != cpp_type_kind::builtin_t
          || static_cast<cpp_builtin_type&>(*underlying_type).builtin_type_kind() != cpp_int;

    auto builder = cpp_enum::builder(std::string(name), is_scoped, std::move(underlying_type),
                                     has_underlying_type);

    auto has_values = false;
    auto inner      = entity["inner"].get_array();
    if (inner.error() != simdjson::NO_SUCH_FIELD)
    {
        for (dom::object child : inner.value())
        {
            auto kind = child["kind"].get_string().value();
            if (kind == "FullComment")
            {
                auto comment = parse_comment(context, child);
                builder.get().set_comment(std::move(comment));
            }
            else
            {
                has_values = true;
                builder.add_value(parse_enum_value(context, child));
            }
        }
    }

    if (is_declaration)
    {
        return builder.finish_declaration(*context.idx, id);
    }
    else
    {
        auto semantic_parent = [&]() -> type_safe::optional<cpp_entity_ref> {
            auto field = entity["parentDeclContextId"].get_string();
            if (field.error() == simdjson::NO_SUCH_FIELD)
                return type_safe::nullopt;

            // TODO: get actual scope name
            return cpp_entity_ref(get_entity_id(context, field.value()), "");
        }();
        return builder.finish(*context.idx, id, type_safe::nullopt);
    }
}

