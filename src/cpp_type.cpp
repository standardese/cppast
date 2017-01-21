// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_type.hpp>

using namespace cppast;

bool detail::cpp_type_ref_predicate::operator()(const cpp_entity& e)
{
    return is_type(e.type());
}
