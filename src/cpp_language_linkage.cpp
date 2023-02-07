// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_language_linkage.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_language_linkage::kind() noexcept
{
    return cpp_entity_kind::language_linkage_t;
}

bool cpp_language_linkage::is_block() const noexcept
{
    if (begin() == end())
    {
        // An empty container must be a "block" of the form: extern "C" {}
        return true;
    }
    return std::next(begin()) != end(); // more than one entity, so block
}

cpp_entity_kind cpp_language_linkage::do_get_entity_kind() const noexcept
{
    return kind();
}
