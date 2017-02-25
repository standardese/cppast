// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_expression.hpp>

#include "parse_functions.hpp"

using namespace cppast;

namespace
{
    std::string get_expression_str(detail::token_stream& stream)
    {
        // just concat everything
        std::string expr;
        while (!stream.done())
            expr += stream.get().c_str();
        return expr;
    }
}

std::unique_ptr<cpp_expression> detail::parse_expression(const detail::parse_context& context,
                                                         const CXCursor&              cur)
{
    auto kind = clang_getCursorKind(cur);
    DEBUG_ASSERT(clang_isExpression(kind), detail::assert_handler{});

    detail::tokenizer    tokenizer(context.tu, context.file, cur);
    detail::token_stream stream(tokenizer, cur);

    auto type = parse_type(context, clang_getCursorType(cur));
    auto expr = get_expression_str(stream);

    if (kind == CXCursor_CharacterLiteral || kind == CXCursor_CompoundLiteralExpr
        || kind == CXCursor_FloatingLiteral || kind == CXCursor_ImaginaryLiteral
        || kind == CXCursor_IntegerLiteral || kind == CXCursor_StringLiteral
        || kind == CXCursor_CXXBoolLiteralExpr || kind == CXCursor_CXXNullPtrLiteralExpr)
        return cpp_literal_expression::build(std::move(type), std::move(expr));
    else
        return cpp_unexposed_expression::build(std::move(type), std::move(expr));
}

std::unique_ptr<cpp_expression> detail::parse_raw_expression(const parse_context& context,
                                                             token_stream&        stream,
                                                             const CXType&        type)
{
    if (stream.done())
        return nullptr;
    auto expr = get_expression_str(stream);
    return cpp_unexposed_expression::build(parse_type(context, type), std::move(expr));
}
