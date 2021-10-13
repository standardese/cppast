// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

using namespace cppast;

std::unique_ptr<cpp_expression> astdump_detail::parse_expression(parse_context& context,
                                                                 dom::value     expr)
{
    auto kind = expr["kind"].get_string().value();
    if (kind == "ImplicitCastExpr")
        // Skip this level of expression.
        return parse_expression(context, expr["inner"].at(0).value());

    auto type = parse_qual_type(context, expr["type"]["qualType"].get_string().value());

    auto value = expr["value"].get_string();
    if (value.error() != simdjson::NO_SUCH_FIELD)
        return cpp_literal_expression::build(std::move(type), std::string(value.value()));
    else
        // TODO: proper unexposed expression
        return cpp_unexposed_expression::build(std::move(type), cpp_token_string({}));
}

