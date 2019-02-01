// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_function_template.hpp>

#include <cppast/cpp_entity_kind.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

namespace
{
template <typename TemplateT, typename EntityT, typename Predicate>
type_safe::optional<typename TemplateT::builder> get_builder(const detail::parse_context& context,
                                                             const CXCursor& cur, Predicate p)
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

    auto entity = detail::parse_entity(context, nullptr, result, cur);
    if (!entity)
        return type_safe::nullopt;
    DEBUG_ASSERT(p(entity->kind()), detail::parse_error_handler{}, cur, "wrong child of template");
    return typename TemplateT::builder(
        std::unique_ptr<EntityT>(static_cast<EntityT*>(entity.release())));
}

std::unique_ptr<cpp_template_parameter> parse_type_parameter(const detail::parse_context& context,
                                                             const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter,
                 detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);
    auto                   name = detail::get_cursor_name(cur);

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

    cpp_attribute_list attributes;
    auto               def = detail::parse_default_value(attributes, context, cur, name.c_str());

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

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

    auto result = cpp_non_type_template_parameter::build(*context.idx, detail::get_entity_id(cur),
                                                         name.c_str(),
                                                         detail::parse_type(context, cur, type),
                                                         is_variadic, std::move(def));
    result->add_attribute(attributes);
    return std::move(result);
}

std::unique_ptr<cpp_template_template_parameter> parse_template_parameter(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter,
                 detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);
    auto                   name = detail::get_cursor_name(cur);

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
                DEBUG_ASSERT(!spelling.empty() && spelling.back() == '>', detail::assert_handler{});
                spelling.pop_back();
                DEBUG_ASSERT(!spelling.empty() && spelling.back() == '>', detail::assert_handler{});
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
void parse_parameters(Builder& builder, const detail::parse_context& context, const CXCursor& cur)
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

void handle_comment_attributes(cpp_entity& templ_entity, cpp_entity& non_template)
{
    // steal comment
    auto comment = type_safe::copy(non_template.comment());
    non_template.set_comment(type_safe::nullopt);
    templ_entity.set_comment(std::move(comment));

    // copy attributes over
    templ_entity.add_attribute(non_template.attributes());
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_alias_template(const detail::parse_context& context,
                                                             const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_TypeAliasTemplateDecl,
                 detail::assert_handler{});
    auto builder
        = get_builder<cpp_alias_template, cpp_type_alias>(context, cur, [](cpp_entity_kind k) {
              return k == cpp_entity_kind::type_alias_t;
          });
    if (!builder)
        return nullptr;
    context.comments.match(builder.value().get(), cur);
    builder.value().get().add_attribute(builder.value().get().begin()->attributes());
    parse_parameters(builder.value(), context, cur);
    return builder.value().finish(*context.idx, detail::get_entity_id(cur),
                                  false); // not a definition
}

std::unique_ptr<cpp_entity> detail::parse_cpp_function_template(
    const detail::parse_context& context, const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_FunctionTemplate, detail::assert_handler{});

    std::unique_ptr<cpp_entity> func;
    switch (clang_getTemplateCursorKind(cur))
    {
    case CXCursor_FunctionDecl:
        func = detail::parse_cpp_function(context, cur, is_friend);
        break;
    case CXCursor_CXXMethod:
        if (auto sfunc = detail::try_parse_static_cpp_function(context, cur))
            func = std::move(sfunc);
        else
            func = detail::parse_cpp_member_function(context, cur, is_friend);
        break;
    case CXCursor_ConversionFunction:
        func = detail::parse_cpp_conversion_op(context, cur, is_friend);
        break;
    case CXCursor_Constructor:
        func = detail::parse_cpp_constructor(context, cur, is_friend);
        break;

    default:
        DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur, "unexpected template cursor kind");
    }

    if (!func)
        return nullptr;
    auto func_ptr = func.get();

    cpp_function_template::builder builder(
        std::unique_ptr<cpp_function_base>(static_cast<cpp_function_base*>(func.release())));
    parse_parameters(builder, context, cur);
    handle_comment_attributes(builder.get(), *func_ptr);
    return builder.finish(*context.idx, detail::get_entity_id(cur),
                          builder.get().function().is_definition());
}

namespace
{
template <class Builder>
void parse_arguments(Builder& b, const detail::parse_context& context, const CXCursor& cur)
{
    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    while (!stream.done() && !detail::skip_if(stream, detail::get_cursor_name(cur).c_str(), true))
        stream.bump();

    if (stream.peek() == "<")
    {
        auto iter = detail::find_closing_bracket(stream);
        stream.bump();

        auto args = detail::to_string(stream, iter);
        b.add_unexposed_arguments(std::move(args));
    }
    else
        b.add_unexposed_arguments(cpp_token_string::builder().finish());
}
} // namespace

std::unique_ptr<cpp_entity> detail::try_parse_cpp_function_template_specialization(
    const detail::parse_context& context, const CXCursor& cur, bool is_friend)
{
    auto templ = clang_getSpecializedCursorTemplate(cur);
    if (clang_Cursor_isNull(templ))
        return nullptr;

    std::unique_ptr<cpp_entity> func;
    switch (clang_getCursorKind(cur))
    {
    case CXCursor_FunctionDecl:
        func = detail::parse_cpp_function(context, cur, is_friend);
        break;
    case CXCursor_CXXMethod:
        if (auto sfunc = detail::try_parse_static_cpp_function(context, cur))
            func = std::move(sfunc);
        else
            func = detail::parse_cpp_member_function(context, cur, is_friend);
        break;
    case CXCursor_ConversionFunction:
        func = detail::parse_cpp_conversion_op(context, cur, is_friend);
        break;
    case CXCursor_Constructor:
        func = detail::parse_cpp_constructor(context, cur, is_friend);
        break;

    default:
        DEBUG_UNREACHABLE(detail::parse_error_handler{}, cur,
                          "unexpected function kind that is being specialized");
    }
    if (!func)
        return nullptr;
    auto func_ptr = func.get();

    cpp_function_template_specialization::builder
        builder(std::unique_ptr<cpp_function_base>(static_cast<cpp_function_base*>(func.release())),
                cpp_template_ref(detail::get_entity_id(templ), ""));
    parse_arguments(builder, context, cur);
    handle_comment_attributes(builder.get(), *func_ptr);
    return builder.finish(*context.idx, detail::get_entity_id(cur),
                          builder.get().function().is_definition());
}

std::unique_ptr<cpp_entity> detail::parse_cpp_class_template(const detail::parse_context& context,
                                                             const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_ClassTemplate, detail::assert_handler{});

    auto c = detail::parse_cpp_class(context, cur, clang_getNullCursor());
    if (!c)
        return nullptr;
    auto c_ptr = c.get();

    cpp_class_template::builder builder(
        std::unique_ptr<cpp_class>(static_cast<cpp_class*>(c.release())));
    handle_comment_attributes(builder.get(), *c_ptr);
    parse_parameters(builder, context, cur);
    return builder.finish(*context.idx, detail::get_entity_id(cur),
                          builder.get().class_().is_definition());
}

std::unique_ptr<cpp_entity> detail::try_parse_full_cpp_class_template_specialization(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_ClassDecl
                     || clang_getCursorKind(cur) == CXCursor_StructDecl
                     || clang_getCursorKind(cur) == CXCursor_UnionDecl,
                 detail::assert_handler{});

    auto templ = clang_getSpecializedCursorTemplate(cur);
    if (clang_Cursor_isNull(templ))
        return nullptr;
    return detail::parse_cpp_class_template_specialization(context, cur);
}

std::unique_ptr<cpp_entity> detail::parse_cpp_class_template_specialization(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_ClassTemplatePartialSpecialization
                     || clang_getCursorKind(cur) == CXCursor_ClassDecl
                     || clang_getCursorKind(cur) == CXCursor_StructDecl
                     || clang_getCursorKind(cur) == CXCursor_UnionDecl,
                 detail::assert_handler{});

    auto primary = clang_getSpecializedCursorTemplate(cur);
    auto c       = detail::parse_cpp_class(context, cur, clang_getNullCursor());
    if (!c)
        return nullptr;
    auto c_ptr = c.get();

    cpp_class_template_specialization::builder
        builder(std::unique_ptr<cpp_class>(static_cast<cpp_class*>(c.release())),
                cpp_template_ref(detail::get_entity_id(primary), ""));
    handle_comment_attributes(builder.get(), *c_ptr);
    parse_parameters(builder, context, cur);
    parse_arguments(builder, context, cur);
    return builder.finish(*context.idx, detail::get_entity_id(cur),
                          builder.get().class_().is_definition());
}
