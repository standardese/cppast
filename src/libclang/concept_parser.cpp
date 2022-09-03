// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_concept.hpp>

#include <cppast/cpp_entity_kind.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

std::unique_ptr<cpp_entity> detail::try_parse_cpp_concept(const detail::parse_context& context,
                                                          const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_UnexposedDecl, detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    if (!detail::skip_if(stream, "template"))
        return nullptr;

    if (stream.peek() != "<")
        return nullptr;


    auto closing_bracket_iter = detail::find_closing_bracket(stream);
    auto params               = to_string(stream, closing_bracket_iter);

    if (!detail::skip_if(stream, ">"))
        return nullptr;

    if (!detail::skip_if(stream, "concept"))
        return nullptr;

    const auto& identifier_token = stream.get();
    if (identifier_token.kind() != CXTokenKind::CXToken_Identifier)
    {
        return nullptr;
    }

    cpp_concept::builder builder(identifier_token.value().std_str());

    if (!detail::skip_if(stream, "="))
    {
        return nullptr;
    }

    if (*(stream.end() - 1) != ";")
    {
        return nullptr;
    }

    builder.set_expression(
        parse_raw_expression(context, stream, stream.end() - 1,
                             cpp_builtin_type::build(cpp_builtin_type_kind::cpp_bool)));

    builder.set_parameters(std::move(params));

    return builder.finish(*context.idx, detail::get_entity_id(cur));
}
