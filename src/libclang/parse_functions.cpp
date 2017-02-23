// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

using namespace cppast;

cpp_entity_id detail::get_entity_id(const CXCursor& cur)
{
    cxstring usr(clang_getCursorUSR(cur));
    DEBUG_ASSERT(!usr.empty(), detail::parse_error_handler{}, cur, "cannot create id for entity");
    return cpp_entity_id(usr.c_str());
}

std::unique_ptr<cpp_entity> detail::parse_entity(const detail::parse_context& context,
                                                 const CXCursor&              cur) try
{
    auto kind = clang_getCursorKind(cur);
    switch (kind)
    {
    case CXCursor_UnexposedDecl:
        // go through all the try_parse_XXX functions
        if (auto entity = try_parse_cpp_language_linkage(context, cur))
            return std::move(entity);
        break;

    case CXCursor_Namespace:
        return parse_cpp_namespace(context, cur);
    case CXCursor_NamespaceAlias:
        return parse_cpp_namespace_alias(context, cur);
    case CXCursor_UsingDirective:
        return parse_cpp_using_directive(context, cur);
    case CXCursor_UsingDeclaration:
        return parse_cpp_using_declaration(context, cur);

    case CXCursor_TypeAliasDecl:
    case CXCursor_TypedefDecl:
        return parse_cpp_type_alias(context, cur);

    case CXCursor_EnumDecl:
        return parse_cpp_enum(context, cur);

    default:
        break;
    }

    auto msg = format("unhandled cursor of kind '", get_cursor_kind_spelling(cur).c_str(), "'");
    context.logger->log("libclang parser",
                        diagnostic{std::move(msg), make_location(cur), severity::warning});

    return nullptr;
}
catch (parse_error& ex)
{
    context.logger->log("libclang parser", ex.get_diagnostic());
    return nullptr;
}
