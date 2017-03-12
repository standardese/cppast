// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_function.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

namespace
{
    std::unique_ptr<cpp_function_parameter> parse_parameter(const detail::parse_context& context,
                                                            const CXCursor&              cur)
    {
        auto name = detail::get_cursor_name(cur);
        auto type = detail::parse_type(context, clang_getCursorType(cur));

        std::unique_ptr<cpp_expression> default_value;
        detail::visit_children(cur, [&](const CXCursor& child) {
            DEBUG_ASSERT(clang_isExpression(child.kind) && !default_value,
                         detail::parse_error_handler{}, child,
                         "unexpected child cursor of function parameter");
            default_value = detail::parse_expression(context, child);
        });

        return cpp_function_parameter::build(*context.idx, detail::get_entity_id(cur), name.c_str(),
                                             std::move(type), std::move(default_value));
    }

    template <class Builder>
    void add_parameters(const detail::parse_context& context, Builder& builder, const CXCursor& cur)
    {
        detail::visit_children(cur, [&](const CXCursor& child) {
            if (clang_getCursorKind(child) != CXCursor_ParmDecl)
                return;

            try
            {
                auto parameter = parse_parameter(context, child);
                builder.add_parameter(std::move(parameter));
            }
            catch (detail::parse_error& ex)
            {
                context.logger->log("libclang parser", ex.get_diagnostic());
            }
        });
    }

    void skip_parameters(detail::token_stream& stream)
    {
        detail::skip_brackets(stream);
    }

    std::unique_ptr<cpp_expression> try_parse_noexcept(detail::token_stream&        stream,
                                                       const detail::parse_context& context)
    {
        if (!detail::skip_if(stream, "noexcept"))
            return nullptr;

        auto type = cpp_builtin_type::build("bool");
        if (stream.peek().value() != "(")
            return cpp_literal_expression::build(std::move(type), "true");

        auto closing = detail::find_closing_bracket(stream);

        detail::skip(stream, "(");
        auto expr = detail::parse_raw_expression(context, stream, closing, std::move(type));
        detail::skip(stream, ")");

        return expr;
    }

    cpp_function_body_kind parse_body_kind(detail::token_stream& stream)
    {
        if (detail::skip_if(stream, "default"))
            return cpp_function_defaulted;
        else if (detail::skip_if(stream, "delete"))
            return cpp_function_deleted;
        DEBUG_UNREACHABLE(detail::parse_error_handler{}, stream.cursor(),
                          "unexpected token for function body kind");
        return cpp_function_declaration;
    }

    std::unique_ptr<cpp_entity> parse_cpp_function_impl(const detail::parse_context& context,
                                                        const CXCursor&              cur)
    {
        auto name = detail::get_cursor_name(cur);

        cpp_function::builder builder(name.c_str(),
                                      detail::parse_type(context, clang_getCursorResultType(cur)));
        add_parameters(context, builder, cur);
        if (clang_Cursor_isVariadic(cur))
            builder.is_variadic();
        builder.storage_class(detail::get_storage_class(cur));

        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);

        // parse prefix
        while (!detail::skip_if(stream, name.c_str()))
        {
            if (detail::skip_if(stream, "constexpr"))
                builder.is_constexpr();
            else
                stream.bump();
        }
        // skip parameters
        skip_parameters(stream);

        auto body =
            clang_isCursorDefinition(cur) ? cpp_function_definition : cpp_function_declaration;
        // parse suffix
        // tokenizer only tokenizes signature, so !stream.done() is sufficient
        while (!stream.done())
        {
            if (auto expr = try_parse_noexcept(stream, context))
                builder.noexcept_condition(std::move(expr));
            else if (skip_if(stream, "="))
                body = parse_body_kind(stream);
            else
                stream.bump();
        }

        return builder.finish(*context.idx, detail::get_entity_id(cur), body);
    }
}

std::unique_ptr<cpp_entity> detail::parse_cpp_function(const detail::parse_context& context,
                                                       const CXCursor&              cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_FunctionDecl, detail::assert_handler{});
    return parse_cpp_function_impl(context, cur);
}

std::unique_ptr<cpp_entity> detail::try_parse_static_cpp_function(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_CXXMethod, detail::assert_handler{});
    if (clang_CXXMethod_isStatic(cur))
        return parse_cpp_function_impl(context, cur);
    return nullptr;
}
