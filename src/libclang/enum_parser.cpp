// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_enum.hpp>

#include "parse_functions.hpp"
#include "libclang_visitor.hpp"

using namespace cppast;

namespace
{
    std::unique_ptr<cpp_enum_value> parse_enum_value(const detail::parse_context& context,
                                                     const CXCursor&              cur)
    {
        DEBUG_ASSERT(cur.kind == CXCursor_EnumConstantDecl, detail::parse_error_handler{}, cur,
                     "unexpected child cursor of enum");

        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);

        // <identifier> [<attribute>],
        // or: <identifier> [<attribute>] = <expression>,
        auto& name = stream.get().value();
        detail::skip_attribute(stream);

        std::unique_ptr<cpp_expression> value;
        if (detail::skip_if(stream, "="))
        {
            detail::visit_children(cur, [&](const CXCursor& child) {
                DEBUG_ASSERT(clang_isExpression(child.kind) && !value,
                             detail::parse_error_handler{}, cur,
                             "unexpected child cursor of enum value");

                value = detail::parse_expression(context, child);
            });
        }

        return cpp_enum_value::build(*context.idx, detail::get_entity_id(cur), name.c_str(),
                                     std::move(value));
    }

    cpp_enum::builder make_enum_builder(const detail::parse_context& context, const CXCursor& cur)
    {
        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);

        // [<attribute>] enum [class] [<attribute>] name [: type] {
        detail::skip_attribute(stream);
        detail::skip(stream, "enum");
        auto scoped = detail::skip_if(stream, "class");
        detail::skip_attribute(stream);
        auto& name = stream.get().value();

        std::unique_ptr<cpp_type> type;
        if (detail::skip_if(stream, ":"))
            // parse type, explictly given one
            type = detail::parse_type(context, clang_getEnumDeclIntegerType(cur));

        return cpp_enum::builder(name.c_str(), scoped, std::move(type));
    }
}

std::unique_ptr<cpp_entity> detail::parse_cpp_enum(const detail::parse_context& context,
                                                   const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_EnumDecl, detail::assert_handler{});

    auto builder = make_enum_builder(context, cur);
    context.comments.match(builder.get(), cur);
    detail::visit_children(cur, [&](const CXCursor& child) {
        try
        {
            auto entity = parse_enum_value(context, child);
            if (entity)
                context.comments.match(*entity, child);
            builder.add_value(std::move(entity));
        }
        catch (parse_error& ex)
        {
            context.logger->log("libclang parser", ex.get_diagnostic());
        }
    });
    if (clang_isCursorDefinition(cur))
        return builder.finish(*context.idx, get_entity_id(cur));
    else
        return builder.finish_declaration(*context.idx, get_entity_id(cur));
}
