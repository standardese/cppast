// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include "parse_functions.hpp"

#include <clang-c/Index.h>
#include <cppast/cpp_language_linkage.hpp>

#include "libclang_visitor.hpp"

using namespace cppast;

std::unique_ptr<cpp_entity> detail::try_parse_cpp_language_linkage(const parse_context& context,
                                                                   const CXCursor&      cur)
{
    // libclang <16 reports 'extern "C"' as CXCursor_UnexposedDecl; libclang 16+
    // gives it its own kind, CXCursor_LinkageSpec. Accept both.
    DEBUG_ASSERT(cur.kind == CXCursor_UnexposedDecl || cur.kind == CXCursor_LinkageSpec,
                 detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    // extern <name> ...
    if (!detail::skip_if(stream, "extern"))
        return nullptr;
    // unexposed variable starting with extern - must be a language linkage
    // (function, variables are not unexposed)
    auto& name = stream.get().value();

    auto builder = cpp_language_linkage::builder(name.c_str());
    context.comments.match(builder.get(), cur);
    detail::visit_children(cur, [&](const CXCursor& child) {
        auto entity = parse_entity(context, &builder.get(), child);
        if (entity)
            builder.add_child(std::move(entity));
    });

    return builder.finish();
}
