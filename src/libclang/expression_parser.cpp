// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_expression.hpp>

#include "parse_functions.hpp"

using namespace cppast;

namespace
{
    std::string get_expression_str(detail::token_stream& stream, detail::token_iterator end)
    {
        // just concat everything
        std::string expr;
        while (stream.cur() != end)
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

    auto type = parse_type(context, cur, clang_getCursorType(cur));
    auto expr = get_expression_str(stream, stream.end());
    if (kind == CXCursor_CallExpr && (expr.empty() || expr.back() != ')'))
    {
        // we have a call expression that doesn't end in a closing parentheses
        // this means default constructor, don't parse it at all
        // so, for example a variable doesn't have a default value
        return nullptr;
    }
    else if (kind == CXCursor_CharacterLiteral || kind == CXCursor_CompoundLiteralExpr
             || kind == CXCursor_FloatingLiteral || kind == CXCursor_ImaginaryLiteral
             || kind == CXCursor_IntegerLiteral || kind == CXCursor_StringLiteral
             || kind == CXCursor_CXXBoolLiteralExpr || kind == CXCursor_CXXNullPtrLiteralExpr)
        return cpp_literal_expression::build(std::move(type), std::move(expr));
    else
        return cpp_unexposed_expression::build(std::move(type), std::move(expr));
}

std::unique_ptr<cpp_expression> detail::parse_raw_expression(const parse_context&,
                                                             token_stream&             stream,
                                                             token_iterator            end,
                                                             std::unique_ptr<cpp_type> type)
{
    if (stream.done())
        return nullptr;
    auto expr = get_expression_str(stream, end);
    return cpp_unexposed_expression::build(std::move(type), std::move(expr));
}
