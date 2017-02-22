// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_language_linkage.hpp>
#include <clang-c/Index.h>

#include "libclang_visitor.hpp"

using namespace cppast;

std::unique_ptr<cpp_entity> detail::try_parse_cpp_language_linkage(const parse_context& context,
                                                                   const CXCursor&      cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_UnexposedDecl,
                 detail::assert_handler{}); // not exposed currently

    detail::tokenizer    tokenizer(context.tu, context.file, cur);
    detail::token_stream stream(tokenizer, cur);

    // extern <name> ...
    if (!detail::skip_if(stream, "extern"))
        return nullptr;
    // unexposed variable starting with extern - must be a language linkage
    // (function, variables are not unexposed)
    auto& name = stream.get().value();

    auto builder = cpp_language_linkage::builder(name.c_str());
    detail::visit_children(cur, [&](const CXCursor& child) {
        auto entity = parse_entity(context, child);
        if (entity)
            builder.add_child(std::move(entity));
    });

    return builder.finish();
}
