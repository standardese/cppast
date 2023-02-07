// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_concept.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cppast::cpp_concept::kind() noexcept
{
    return cpp_entity_kind::concept_t;
}

cpp_entity_kind cpp_concept::do_get_entity_kind() const noexcept
{
    return cpp_entity_kind::concept_t;
}

std::unique_ptr<cpp_concept> cpp_concept::builder::finish(const cpp_entity_index& idx,
                                                          cpp_entity_id           id)
{
    idx.register_definition(id, type_safe::ref(*concept_));
    return std::move(concept_);
}
