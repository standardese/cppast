// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_storage_class_specifiers.hpp>

using namespace cppast;

cpp_entity_id detail::get_entity_id(const CXCursor& cur)
{
    cxstring usr(clang_getCursorUSR(cur));
    DEBUG_ASSERT(!usr.empty(), detail::parse_error_handler{}, cur, "cannot create id for entity");
    return cpp_entity_id(usr.c_str());
}

detail::cxstring detail::get_cursor_name(const CXCursor& cur)
{
    return cxstring(clang_getCursorSpelling(cur));
}

cpp_storage_class_specifiers detail::get_storage_class(const CXCursor& cur)
{
    switch (clang_Cursor_getStorageClass(cur))
    {
    case CX_SC_Invalid:
        break;

    case CX_SC_None:
        return cpp_storage_class_none;

    case CX_SC_Auto:
    case CX_SC_Register:
        return cpp_storage_class_auto;

    case CX_SC_Extern:
        return cpp_storage_class_extern;
    case CX_SC_Static:
        return cpp_storage_class_static;

    case CX_SC_PrivateExtern:
    case CX_SC_OpenCLWorkGroupLocal:
        // non-exposed storage classes
        return cpp_storage_class_auto;
    }

    DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur, "unexpected storage class");
    return cpp_storage_class_auto;
}

void detail::comment_context::match(cpp_entity& e, const CXCursor& cur) const
{
    auto     pos = clang_getRangeStart(clang_getCursorExtent(cur));
    unsigned line;
    clang_getSpellingLocation(pos, nullptr, &line, nullptr, nullptr);

    match(e, line);
}

void detail::comment_context::match(cpp_entity& e, unsigned line) const
{
    // find comment
    while (cur_ != end_ && cur_->line + 1 < line)
        ++cur_;
    if (cur_ != end_ && cur_->matches(e, line))
        e.set_comment(std::move(cur_++->comment));
}

std::unique_ptr<cpp_entity> detail::parse_entity(const detail::parse_context& context,
                                                 const CXCursor& cur, bool as_template) try
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
        return parse_cpp_type_alias(context, cur, as_template);
    case CXCursor_EnumDecl:
        return parse_cpp_enum(context, cur);
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
        return parse_cpp_class(context, cur);

    case CXCursor_VarDecl:
        return parse_cpp_variable(context, cur);
    case CXCursor_FieldDecl:
        return parse_cpp_member_variable(context, cur);

    case CXCursor_FunctionDecl:
        return parse_cpp_function(context, cur);
    case CXCursor_CXXMethod:
        // check for static function
        if (auto func = try_parse_static_cpp_function(context, cur))
            return func;
        return parse_cpp_member_function(context, cur);
    case CXCursor_ConversionFunction:
        return parse_cpp_conversion_op(context, cur);
    case CXCursor_Constructor:
        return parse_cpp_constructor(context, cur);
    case CXCursor_Destructor:
        return parse_cpp_destructor(context, cur);

    case CXCursor_TypeAliasTemplateDecl:
        return parse_cpp_alias_template(context, cur);

    default:
        break;
    }

    auto msg = detail::format("unhandled cursor of kind '",
                              detail::get_cursor_kind_spelling(cur).c_str(), "'");
    context.logger->log("libclang parser",
                        diagnostic{std::move(msg), detail::make_location(cur), severity::warning});

    return nullptr;
}
catch (parse_error& ex)
{
    context.logger->log("libclang parser", ex.get_diagnostic());
    return nullptr;
}
