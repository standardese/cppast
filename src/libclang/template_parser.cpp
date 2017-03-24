// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_alias_template.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

namespace
{
    template <typename TemplateT, typename EntityT>
    type_safe::optional<typename TemplateT::builder> get_builder(
        const detail::parse_context& context, const CXCursor& cur)
    {
        // we need the actual entity first, then the parameters
        // so two visit calls are required

        auto result = clang_getNullCursor();
        detail::visit_children(cur, [&](const CXCursor& child) {
            auto kind = clang_getCursorKind(child);
            if (kind == CXCursor_TemplateTypeParameter || kind == CXCursor_NonTypeTemplateParameter
                || kind == CXCursor_TemplateTemplateParameter)
                return;
            DEBUG_ASSERT(clang_Cursor_isNull(result), detail::parse_error_handler{}, cur,
                         "unexpected child of template");
            result = child;
        });
        DEBUG_ASSERT(!clang_Cursor_isNull(result), detail::parse_error_handler{}, cur,
                     "missing child of template");

        auto entity = detail::parse_entity(context, result, cur);
        if (!entity)
            return type_safe::nullopt;
        DEBUG_ASSERT(entity->kind() == EntityT::kind(), detail::parse_error_handler{}, cur,
                     "wrong child of template");
        return typename TemplateT::builder(
            std::unique_ptr<EntityT>(static_cast<EntityT*>(entity.release())));
    }

    std::unique_ptr<cpp_template_parameter> parse_type_parameter(
        const detail::parse_context& context, const CXCursor& cur)
    {
        DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter,
                     detail::assert_handler{});

        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);
        auto                 name = detail::get_cursor_name(cur);

        // syntax: typename/class [...] name [= ...]
        auto keyword = cpp_template_keyword::keyword_class;
        if (detail::skip_if(stream, "typename"))
            keyword = cpp_template_keyword::keyword_typename;
        else
            detail::skip(stream, "class");

        auto variadic = false;
        if (detail::skip_if(stream, "..."))
            variadic = true;

        if (stream.peek() != "=")
            detail::skip(stream, name.c_str());

        std::unique_ptr<cpp_type> def;
        if (detail::skip_if(stream, "="))
            // default type
            def = detail::parse_raw_type(context, stream, stream.end());

        return cpp_template_type_parameter::build(*context.idx, detail::get_entity_id(cur),
                                                  name.c_str(), keyword, variadic, std::move(def));
    }

    std::unique_ptr<cpp_template_parameter> parse_non_type_parameter(
        const detail::parse_context& context, const CXCursor& cur)
    {
        DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter,
                     detail::assert_handler{});

        auto name = detail::get_cursor_name(cur);
        auto type = clang_getCursorType(cur);

        std::unique_ptr<cpp_expression> def;
        detail::visit_children(cur, [&](const CXCursor& child) {
            DEBUG_ASSERT(clang_isExpression(clang_getCursorKind(child)) && !def,
                         detail::parse_error_handler{}, cur,
                         "unexpected child cursor of non type template parameter");
            def = detail::parse_expression(context, child);
        });

        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);

        // see if it is variadic
        // syntax a): some-tokens ... name some-tokens
        // syntax b): some-tokens (some-tokens ... name) some-tokens-or-...
        // name might be empty, so can't loop until it occurs
        // some-tokens will not contain ... or parenthesis, however

        auto is_variadic = false;
        for (; !stream.done(); stream.bump())
        {
            if (stream.peek() == "...")
            {
                is_variadic = true;
                break;
            }
            else if (stream.peek() == ")")
                break;
        }

        return cpp_non_type_template_parameter::build(*context.idx, detail::get_entity_id(cur),
                                                      name.c_str(),
                                                      detail::parse_type(context, cur, type),
                                                      is_variadic, std::move(def));
    }

    std::unique_ptr<cpp_template_template_parameter> parse_template_parameter(
        const detail::parse_context& context, const CXCursor& cur)
    {
        DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter,
                     detail::assert_handler{});

        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);
        auto                 name = detail::get_cursor_name(cur);

        // syntax: template <…> class/typename [...] name [= …]
        detail::skip(stream, "template");
        detail::skip_brackets(stream);

        auto keyword = cpp_template_keyword::keyword_class;
        if (detail::skip_if(stream, "typename"))
            keyword = cpp_template_keyword::keyword_typename;
        else
            detail::skip(stream, "class");

        auto is_variadic = detail::skip_if(stream, "...");
        detail::skip(stream, name.c_str());

        // now we can create the builder
        cpp_template_template_parameter::builder builder(name.c_str(), is_variadic);
        builder.keyword(keyword);

        // look for parameters and default
        detail::visit_children(cur, [&](const CXCursor& child) {
            auto kind = clang_getCursorKind(child);
            if (kind == CXCursor_TemplateTypeParameter)
                builder.add_parameter(parse_type_parameter(context, child));
            else if (kind == CXCursor_NonTypeTemplateParameter)
                builder.add_parameter(parse_non_type_parameter(context, child));
            else if (kind == CXCursor_TemplateTemplateParameter)
                builder.add_parameter(parse_template_parameter(context, child));
            else if (kind == CXCursor_TemplateRef)
            {
                auto target = clang_getCursorReferenced(child);

                // stream is after the keyword
                // syntax: = default
                detail::skip(stream, "=");

                std::string spelling;
                while (!stream.done())
                    spelling += stream.get().c_str();
                if (stream.unmunch())
                {
                    DEBUG_ASSERT(!spelling.empty() && spelling.back() == '>',
                                 detail::assert_handler{});
                    spelling.pop_back();
                    DEBUG_ASSERT(!spelling.empty() && spelling.back() == '>',
                                 detail::assert_handler{});
                }

                builder.default_template(
                    cpp_template_ref(detail::get_entity_id(target), std::move(spelling)));
            }
            else
                DEBUG_ASSERT(clang_isReference(kind), detail::parse_error_handler{}, cur,
                             "unexpected child of template template parameter");
        });

        return builder.finish(*context.idx, detail::get_entity_id(cur));
    }

    template <class Builder>
    void parse_parameters(Builder& builder, const detail::parse_context& context,
                          const CXCursor& cur)
    {
        // now visit to get the parameters
        detail::visit_children(cur, [&](const CXCursor& child) {
            auto kind = clang_getCursorKind(child);
            if (kind == CXCursor_TemplateTypeParameter)
                builder.add_parameter(parse_type_parameter(context, child));
            else if (kind == CXCursor_NonTypeTemplateParameter)
                builder.add_parameter(parse_non_type_parameter(context, child));
            else if (kind == CXCursor_TemplateTemplateParameter)
                builder.add_parameter(parse_template_parameter(context, child));
        });
    }
}

std::unique_ptr<cpp_entity> detail::parse_cpp_alias_template(const detail::parse_context& context,
                                                             const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TypeAliasTemplateDecl,
                 detail::assert_handler{});
    auto builder = get_builder<cpp_alias_template, cpp_type_alias>(context, cur);
    if (!builder)
        return nullptr;
    context.comments.match(builder.value().get(), cur);
    parse_parameters(builder.value(), context, cur);
    return builder.value().finish(*context.idx, detail::get_entity_id(cur));
}
