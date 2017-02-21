// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include "parse_error.hpp"

using namespace cppast;

std::unique_ptr<cpp_entity> detail::parse_entity(const detail::parse_context& context,
                                                 const CXCursor&              cur) try
{
    return nullptr;
}
catch (parse_error& ex)
{
    context.logger->log("libclang parser", ex.get_diagnostic());
    return nullptr;
}
