// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_enum.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

namespace
{
std::unique_ptr<cpp_enum_value> parse_enum_value(const detail::parse_context& context,
                                                 const CXCursor&              cur)
{
    if (clang_isAttribute(clang_getCursorKind(cur)))
        return nullptr;

    DEBUG_ASSERT(cur.kind == CXCursor_EnumConstantDecl, detail::parse_error_handler{}, cur,
                 "unexpected child cursor of enum");

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // <identifier> [<attribute>],
    // or: <identifier> [<attribute>] = <expression>,
    auto& name       = stream.get().value();
    auto  attributes = detail::parse_attributes(stream);

    std::unique_ptr<cpp_expression> value;
    if (detail::skip_if(stream, "="))
    {
        detail::visit_children(cur, [&](const CXCursor& child) {
            DEBUG_ASSERT(clang_isExpression(child.kind) && !value, detail::parse_error_handler{},
                         cur, "unexpected child cursor of enum value");

            value = detail::parse_expression(context, child);
        });
    }

    auto result = cpp_enum_value::build(*context.idx, detail::get_entity_id(cur), name.c_str(),
                                        std::move(value));
    result->add_attribute(attributes);
    return result;
}

cpp_enum::builder make_enum_builder(const detail::parse_context& context, const CXCursor& cur,
                                    type_safe::optional<cpp_entity_ref>& semantic_parent)
{
    auto                   name = detail::get_cursor_name(cur);
    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // enum [class/struct] [<attribute>] name [: type] {
    detail::skip(stream, "enum");
    auto scoped     = detail::skip_if(stream, "class") || detail::skip_if(stream, "struct");
    auto attributes = detail::parse_attributes(stream);

    std::string scope;
    while (!detail::skip_if(stream, name.c_str()))
        if (!detail::append_scope(stream, scope))
            DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur, "unexpected tokens in enum name");
    if (!scope.empty())
        semantic_parent = cpp_entity_ref(detail::get_entity_id(clang_getCursorSemanticParent(cur)),
                                         std::move(scope));

    // parse type
    auto type       = detail::parse_type(context, cur, clang_getEnumDeclIntegerType(cur));
    auto type_given = detail::skip_if(stream, ":");

    auto result = cpp_enum::builder(name.c_str(), scoped, std::move(type), type_given);
    result.get().add_attribute(attributes);
    return result;
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_enum(const detail::parse_context& context,
                                                   const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_EnumDecl, detail::assert_handler{});

    type_safe::optional<cpp_entity_ref> semantic_parent;
    auto                                builder = make_enum_builder(context, cur, semantic_parent);
    context.comments.match(builder.get(), cur);
    detail::visit_children(cur, [&](const CXCursor& child) {
        try
        {
            auto entity = parse_enum_value(context, child);
            if (entity)
            {
                context.comments.match(*entity, child);
                builder.add_value(std::move(entity));
            }
        }
        catch (parse_error& ex)
        {
            context.error = true;
            context.logger->log("libclang parser", ex.get_diagnostic(context.file));
        }
        catch (std::logic_error& ex)
        {
            context.error = true;
            auto location = make_location(child);
            context.logger->log("libclang parser",
                                diagnostic{ex.what(), location, severity::error});
        }
    });
    if (clang_isCursorDefinition(cur))
        return builder.finish(*context.idx, get_entity_id(cur), std::move(semantic_parent));
    else
        return builder.finish_declaration(*context.idx, get_entity_id(cur));
}
