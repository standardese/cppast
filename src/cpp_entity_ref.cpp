// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity_ref.hpp>

#include <cppast/cpp_entity.hpp>

using namespace cppast;

void detail::check_entity_cast(const cpp_entity& e, cpp_entity_kind expected_kind)
{
    DEBUG_ASSERT(e.kind() == expected_kind, detail::precondition_error_handler{},
                 "mismatched entity kind");
}
