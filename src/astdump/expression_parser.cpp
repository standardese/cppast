// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

using namespace cppast;

std::unique_ptr<cpp_expression> astdump_detail::parse_expression(parse_context& context,
                                                                 dom::value     expr)
{
    // TODO: get proper type
    // TODO: proper unexposed expression
    auto kind = expr["kind"].get_string().value();
    auto type = expr["type"]["qualType"].get_string().value();

    auto value = expr["value"].get_string();
    if (value.error() != simdjson::NO_SUCH_FIELD)
        return cpp_literal_expression::build(cpp_unexposed_type::build(std::string(type)),
                                             std::string(value.value()));
    else
        return cpp_unexposed_expression::build(cpp_unexposed_type::build(""), cpp_token_string({}));
}

