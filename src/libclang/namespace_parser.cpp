// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_namespace.hpp>

#include "libclang_visitor.hpp"

using namespace cppast;

namespace
{
    cpp_namespace::builder make_builder(const detail::parse_context& context, const CXCursor& cur)
    {
        detail::tokenizer    tokenizer(context.tu, context.file, cur);
        detail::token_stream stream(tokenizer, cur);
        // [inline] namespace [<attribute>] <identifier> {

        auto is_inline = false;
        if (skip_if(stream, "inline"))
            is_inline = true;

        skip(stream, "namespace");
        skip_attribute(stream);

        // <identifier> {
        auto& name = stream.get().value();
        skip(stream, "{");

        return cpp_namespace::builder(name.c_str(), is_inline);
    }
}

std::unique_ptr<cpp_entity> detail::parse_cpp_namespace(const detail::parse_context& context,
                                                        const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_Namespace, detail::assert_handler{});

    auto builder = make_builder(context, cur);
    detail::visit_children(cur, [&](const CXCursor& cur) {
        auto entity = parse_entity(context, cur);
        if (entity)
            builder.add_child(std::move(entity));
    });
    return builder.finish(*context.idx, get_entity_id(cur));
}
