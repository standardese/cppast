// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_namespace.hpp>
#include <type_safe/deferred_construction.hpp>

#include "libclang_visitor.hpp"

using namespace cppast;

namespace
{
cpp_namespace::builder make_ns_builder(const detail::parse_context& context, const CXCursor& cur)
{
    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);
    // [inline] namespace|:: [<attribute>] <identifier> [{]

    auto is_inline = false;
    if (skip_if(stream, "inline"))
        is_inline = true;

    // C++17 nested namespace declarations get one cursor per nested name.
    // The first cursor starts with the `namespace` keyword, and the
    // following start with the `::` separator. Either way, it is skipped.
    auto is_nested = false;
    if (!detail::skip_if(stream, "namespace"))
    {
        is_nested = true;
        skip(stream, "::");
    }

    auto attributes = parse_attributes(stream);

    // <identifier> {
    // or when anonymous: {
    if (detail::skip_if(stream, "{"))
        return cpp_namespace::builder("", is_inline, false);

    auto& name = stream.get().value();

    auto other_attributes = parse_attributes(stream);
    attributes.insert(attributes.end(), other_attributes.begin(), other_attributes.end());

    // If the next token is not `::`, there are no more nested namespace
    // names, and we expect to see an opening brace.
    if (!detail::skip_if(stream, "::"))
        skip(stream, "{");

    auto result = cpp_namespace::builder(name.c_str(), is_inline, is_nested);
    result.get().add_attribute(attributes);
    return result;
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_namespace(const detail::parse_context& context,
                                                        cpp_entity& parent, const CXCursor& cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_Namespace, detail::assert_handler{});

    auto builder = make_ns_builder(context, cur);
    if (builder.get().is_nested())
    {
        // steal comment from parent
        DEBUG_ASSERT(parent.kind() == cpp_namespace::kind(), detail::assert_handler{});
        builder.get().set_comment(type_safe::copy(parent.comment()));
        parent.set_comment(type_safe::nullopt);
    }
    else
        context.comments.match(builder.get(), cur);

    detail::visit_children(cur, [&](const CXCursor& cur) {
        auto entity = parse_entity(context, &builder.get(), cur);
        if (entity)
            builder.add_child(std::move(entity));
    });
    return builder.finish(*context.idx, get_entity_id(cur));
}

namespace
{
cpp_entity_id parse_ns_target_cursor(const CXCursor& cur)
{
    cpp_entity_id result("");
    detail::visit_children(cur,
                           [&](const CXCursor& child) {
                               auto referenced = clang_getCursorReferenced(child);
                               auto kind       = clang_getCursorKind(referenced);
                               if (kind == CXCursor_Namespace)
                                   result = detail::get_entity_id(referenced);
                               else if (kind == CXCursor_NamespaceAlias)
                                   // get target of namespace alias instead
                                   result = parse_ns_target_cursor(referenced);
                               else
                                   DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur,
                                                     "unexpected target for namespace "
                                                     "alias/using directive");
                           },
                           true);
    return result;
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_namespace_alias(const detail::parse_context& context,
                                                              const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_NamespaceAlias, detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // namespace <identifier> = <nested identifier>;
    detail::skip(stream, "namespace");
    auto name = stream.get().c_str();
    detail::skip(stream, "=");

    // <nested identifier>;
    std::string target_name;
    while (!stream.done() && !detail::skip_if(stream, ";"))
        target_name += stream.get().c_str();

    auto target = cpp_namespace_ref(parse_ns_target_cursor(cur), std::move(target_name));
    auto result = cpp_namespace_alias::build(*context.idx, get_entity_id(cur), std::move(name),
                                             std::move(target));
    context.comments.match(*result, cur);
    return result;
}

std::unique_ptr<cpp_entity> detail::parse_cpp_using_directive(const detail::parse_context& context,
                                                              const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_UsingDirective, detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // using namespace <nested identifier>;
    detail::skip(stream, "using");
    detail::skip(stream, "namespace");

    // <nested identifier>;
    std::string target_name;
    while (!stream.done() && !detail::skip_if(stream, ";"))
        target_name += stream.get().c_str();

    auto target = cpp_namespace_ref(parse_ns_target_cursor(cur), std::move(target_name));
    auto result = cpp_using_directive::build(target);
    context.comments.match(*result, cur);
    return result;
}

namespace
{
cpp_entity_ref parse_entity_target_cursor(const CXCursor& cur, std::string name)
{
    type_safe::deferred_construction<cpp_entity_ref> result;
    detail::visit_children(cur,
                           [&](const CXCursor& child) {
                               if (result)
                                   return;

                               switch (clang_getCursorKind(child))
                               {
                               case CXCursor_TypeRef:
                               case CXCursor_TemplateRef:
                               case CXCursor_MemberRef:
                               case CXCursor_VariableRef:
                               case CXCursor_DeclRefExpr:
                               {
                                   auto referenced = clang_getCursorReferenced(child);
                                   result = cpp_entity_ref(detail::get_entity_id(referenced),
                                                           std::move(name));
                                   break;
                               }

                               case CXCursor_OverloadedDeclRef:
                               {
                                   auto size = clang_getNumOverloadedDecls(child);
                                   DEBUG_ASSERT(size >= 1u, detail::parse_error_handler{}, cur,
                                                "no target for using declaration");
                                   std::vector<cpp_entity_id> ids;
                                   for (auto i = 0u; i != size; ++i)
                                       ids.push_back(detail::get_entity_id(
                                           clang_getOverloadedDecl(child, i)));
                                   result = cpp_entity_ref(std::move(ids), std::move(name));
                                   break;
                               }

                               case CXCursor_NamespaceRef:
                                   break; // wait for children

                               default:
                                   DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur,
                                                     "unexpected target for using declaration");
                               }
                           },
                           true);
    return result.value();
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_using_declaration(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_UsingDeclaration, detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // using <nested identifier>;
    detail::skip(stream, "using");

    // <nested identifier>;
    std::string target_name;
    while (!stream.done() && !detail::skip_if(stream, ";"))
        target_name += stream.get().c_str();

    auto target = parse_entity_target_cursor(cur, std::move(target_name));
    auto result = cpp_using_declaration::build(std::move(target));
    context.comments.match(*result, cur);
    return result;
}
