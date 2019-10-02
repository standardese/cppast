// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_static_assert.hpp>
#include <cppast/cpp_storage_class_specifiers.hpp>

#include "libclang_visitor.hpp"

using namespace cppast;

cpp_entity_id detail::get_entity_id(const CXCursor& cur)
{
    cxstring usr(clang_getCursorUSR(cur));
    DEBUG_ASSERT(!usr.empty(), detail::parse_error_handler{}, cur, "cannot create id for entity");
    if (clang_getCursorKind(cur) == CXCursor_FunctionTemplate
        || clang_getCursorKind(cur) == CXCursor_ConversionFunction)
    {
        // we have a function template
        // combine return type with USR to ensure no ambiguity when SFINAE is applied there
        // (and hope this prevents all collisions...)
        // same workaround also applies to conversion functions,
        // there template arguments in the result are ignored
        cxstring type_spelling(clang_getTypeSpelling(clang_getCursorResultType(cur)));
        return cpp_entity_id(std::string(usr.c_str()) + type_spelling.c_str());
    }
    else if (clang_getCursorKind(cur) == CXCursor_ClassTemplatePartialSpecialization)
    {
        // libclang issue: templ<T()> vs templ<T() &>
        // but identical USR
        // same workaround: combine display name with usr
        // (and hope this prevents all collisions...)
        cxstring display_name(clang_getCursorDisplayName(cur));
        return cpp_entity_id(std::string(usr.c_str()) + display_name.c_str());
    }
    else
        return cpp_entity_id(usr.c_str());
}

detail::cxstring detail::get_cursor_name(const CXCursor& cur)
{
    return cxstring(clang_getCursorSpelling(cur));
}

cpp_storage_class_specifiers detail::get_storage_class(const CXCursor& cur)
{
    if (clang_getTemplateCursorKind(cur) != CXCursor_NoDeclFound)
        return cpp_storage_class_none;

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
        return cpp_storage_class_none;
    }

    DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur, "unexpected storage class");
    return cpp_storage_class_none;
}

void detail::comment_context::match(cpp_entity& e, const CXCursor& cur) const
{
    auto     pos = clang_getRangeStart(clang_getCursorExtent(cur));
    unsigned line;
    clang_getPresumedLocation(pos, nullptr, &line, nullptr);

    match(e, line);
}

void detail::comment_context::match(cpp_entity& e, unsigned line, bool skip_comments) const
{
    // find comment
    auto save = cur_;
    while (cur_ != end_ && cur_->line + 1 < line)
        ++cur_;
    if (cur_ != end_ && cur_->matches(e, line))
        e.set_comment(std::move(cur_++->comment));

    if (!skip_comments)
        cur_ = save;
}

namespace
{
bool is_friend(const CXCursor& parent_cur)
{
    return clang_getCursorKind(parent_cur) == CXCursor_FriendDecl;
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_entity(const detail::parse_context& context,
                                                 cpp_entity* parent, const CXCursor& cur,
                                                 const CXCursor& parent_cur) try
{
    if (context.logger->is_verbose())
    {
        context.logger->log("libclang parser",
                            format_diagnostic(severity::debug, detail::make_location(cur),
                                              "parsing cursor of type '",
                                              detail::get_cursor_kind_spelling(cur).c_str(), "'"));
    }

    auto kind = clang_getCursorKind(cur);
    switch (kind)
    {
    case CXCursor_UnexposedDecl:
        // go through all the try_parse_XXX functions
        if (auto entity = try_parse_cpp_language_linkage(context, cur))
            return entity;
        break;

    case CXCursor_MacroDefinition:
    case CXCursor_InclusionDirective:
        DEBUG_UNREACHABLE(detail::assert_handler{}, "handle preprocessor in parser callback");
        break;

    case CXCursor_Namespace:
        DEBUG_ASSERT(parent, detail::assert_handler{});
        return parse_cpp_namespace(context, *parent, cur);
    case CXCursor_NamespaceAlias:
        return parse_cpp_namespace_alias(context, cur);
    case CXCursor_UsingDirective:
        return parse_cpp_using_directive(context, cur);
    case CXCursor_UsingDeclaration:
        return parse_cpp_using_declaration(context, cur);

    case CXCursor_TypeAliasDecl:
    case CXCursor_TypedefDecl:
        return parse_cpp_type_alias(context, cur, parent_cur);
    case CXCursor_EnumDecl:
        return parse_cpp_enum(context, cur);
    case CXCursor_ClassDecl:
    case CXCursor_StructDecl:
    case CXCursor_UnionDecl:
        if (auto spec = try_parse_full_cpp_class_template_specialization(context, cur))
            return spec;
        return parse_cpp_class(context, cur, parent_cur);

    case CXCursor_VarDecl:
        return parse_cpp_variable(context, cur);
    case CXCursor_FieldDecl:
        return parse_cpp_member_variable(context, cur);

    case CXCursor_FunctionDecl:
        if (auto tfunc
            = try_parse_cpp_function_template_specialization(context, cur, is_friend(parent_cur)))
            return tfunc;
        return parse_cpp_function(context, cur, is_friend(parent_cur));
    case CXCursor_CXXMethod:
        if (auto tfunc
            = try_parse_cpp_function_template_specialization(context, cur, is_friend(parent_cur)))
            return tfunc;
        else if (auto func = try_parse_static_cpp_function(context, cur))
            return func;
        return parse_cpp_member_function(context, cur, is_friend(parent_cur));
    case CXCursor_ConversionFunction:
        if (auto tfunc
            = try_parse_cpp_function_template_specialization(context, cur, is_friend(parent_cur)))
            return tfunc;
        return parse_cpp_conversion_op(context, cur, is_friend(parent_cur));
    case CXCursor_Constructor:
        if (auto tfunc
            = try_parse_cpp_function_template_specialization(context, cur, is_friend(parent_cur)))
            return tfunc;
        return parse_cpp_constructor(context, cur, is_friend(parent_cur));
    case CXCursor_Destructor:
        return parse_cpp_destructor(context, cur, is_friend(parent_cur));

    case CXCursor_FriendDecl:
        return parse_cpp_friend(context, cur);

    case CXCursor_TypeAliasTemplateDecl:
        return parse_cpp_alias_template(context, cur);
    case CXCursor_FunctionTemplate:
        return parse_cpp_function_template(context, cur, is_friend(parent_cur));
    case CXCursor_ClassTemplate:
        return parse_cpp_class_template(context, cur);
    case CXCursor_ClassTemplatePartialSpecialization:
        return parse_cpp_class_template_specialization(context, cur);

    case CXCursor_StaticAssert:
        return parse_cpp_static_assert(context, cur);

    default:
        break;
    }

    if (!clang_isAttribute(clang_getCursorKind(cur)))
    {
        // build unexposed entity
        detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
        detail::cxtoken_stream stream(tokenizer, cur);
        auto                   spelling = detail::to_string(stream, stream.end());
        if (spelling.begin() + 1 == spelling.end() && spelling.front().spelling == ";")
            // unnecessary semicolon
            return nullptr;

        auto name = detail::get_cursor_name(cur);

        std::unique_ptr<cppast::cpp_entity> entity;
        if (name.empty())
            entity = cpp_unexposed_entity::build(std::move(spelling));
        else
            entity = cpp_unexposed_entity::build(*context.idx, detail::get_entity_id(cur),
                                                 name.c_str(), std::move(spelling));

        context.comments.match(*entity, cur);

        auto msg = detail::format("unhandled cursor of kind '",
                                  detail::get_cursor_kind_spelling(cur).c_str(), "'");
        context.logger->log("libclang parser",
                            format_diagnostic(severity::warning, detail::make_location(cur),
                                              "unhandled cursor of kind '",
                                              detail::get_cursor_kind_spelling(cur).c_str(), "'"));

        return entity;
    }
    else
        return nullptr;
}
catch (parse_error& ex)
{
    context.error = true;
    context.logger->log("libclang parser", ex.get_diagnostic(context.file));
    return nullptr;
}
catch (std::logic_error& ex)
{
    context.error = true;
    context.logger->log("libclang parser",
                        diagnostic{ex.what(), detail::make_location(context.file, cur),
                                   severity::error});
    return nullptr;
}

std::unique_ptr<cpp_entity> detail::parse_cpp_static_assert(const detail::parse_context& context,
                                                            const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_StaticAssert, detail::assert_handler{});

    std::unique_ptr<cpp_expression> expr;
    std::string                     msg;
    detail::visit_children(cur, [&](const CXCursor& child) {
        if (!expr)
        {
            DEBUG_ASSERT(clang_isExpression(clang_getCursorKind(child)),
                         detail::parse_error_handler{}, cur,
                         "unexpected child cursor of static assert");
            expr = detail::parse_expression(context, child);
        }
        else if (msg.empty())
        {
            DEBUG_ASSERT(clang_getCursorKind(child) == CXCursor_StringLiteral,
                         detail::parse_error_handler{}, cur,
                         "unexpected child cursor of static assert");
            msg = detail::get_cursor_name(child).c_str();
            DEBUG_ASSERT(msg.front() == '"' && msg.back() == '"', detail::parse_error_handler{},
                         cur, "unexpected name format");
            msg.pop_back();
            msg.erase(msg.begin());
        }
        else
            DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur,
                              "unexpected child cursor of static assert");
    });

    auto result = cpp_static_assert::build(std::move(expr), std::move(msg));
    context.comments.match(*result, cur);
    return result;
}
