// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_language_linkage.hpp>

using namespace cppast;

std::unique_ptr<cpp_entity> astdump_detail::parse_language_linkage(parse_context& context,
                                                                   dom::object    entity)
{
    auto name = entity["language"].get_string().value();
    // For compatibility with libclang, we need the quotes.
    cpp_language_linkage::builder builder('"' + std::string{name} + '"');

    for (dom::object child : entity["inner"])
    {
        auto e = astdump_detail::parse_entity(context, builder.get(), child);
        if (e)
            builder.add_child(std::move(e));
    }

    return builder.finish();
}

