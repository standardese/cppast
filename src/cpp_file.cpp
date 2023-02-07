// Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include <cppast/cpp_file.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_file::kind() noexcept
{
    return cpp_entity_kind::file_t;
}

cpp_entity_kind cpp_file::do_get_entity_kind() const noexcept
{
    return kind();
}

bool detail::cpp_file_ref_predicate::operator()(const cpp_entity& e)
{
    return e.kind() == cpp_entity_kind::file_t;
}
