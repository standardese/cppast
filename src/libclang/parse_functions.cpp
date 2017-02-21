// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

using namespace cppast;

cpp_entity_id detail::get_entity_id(const CXCursor& cur)
{
    cxstring usr(clang_getCursorUSR(cur));
    DEBUG_ASSERT(!usr.empty(), detail::parse_error_handler{}, cur, "cannot create id for entity");
    return cpp_entity_id(usr.c_str());
}

std::unique_ptr<cpp_entity> detail::parse_entity(const detail::parse_context& context,
                                                 const CXCursor&              cur) try
{
    auto kind = clang_getCursorKind(cur);
    switch (kind)
    {
    case CXCursor_Namespace:
        return parse_cpp_namespace(context, cur);

    default:
        break;
    }

    return nullptr;
}
catch (parse_error& ex)
{
    context.logger->log("libclang parser", ex.get_diagnostic());
    return nullptr;
}
