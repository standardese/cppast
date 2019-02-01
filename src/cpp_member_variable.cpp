// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_member_variable.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

cpp_entity_kind cpp_member_variable::kind() noexcept
{
    return cpp_entity_kind::member_variable_t;
}

std::unique_ptr<cpp_member_variable> cpp_member_variable::build(const cpp_entity_index& idx,
                                                                cpp_entity_id id, std::string name,
                                                                std::unique_ptr<cpp_type> type,
                                                                std::unique_ptr<cpp_expression> def,
                                                                bool is_mutable)
{
    auto result = std::unique_ptr<cpp_member_variable>(
        new cpp_member_variable(std::move(name), std::move(type), std::move(def), is_mutable));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

cpp_entity_kind cpp_member_variable::do_get_entity_kind() const noexcept
{
    return kind();
}

cpp_entity_kind cpp_bitfield::kind() noexcept
{
    return cpp_entity_kind::bitfield_t;
}

std::unique_ptr<cpp_bitfield> cpp_bitfield::build(const cpp_entity_index& idx, cpp_entity_id id,
                                                  std::string name, std::unique_ptr<cpp_type> type,
                                                  unsigned no_bits, bool is_mutable)
{
    auto result = std::unique_ptr<cpp_bitfield>(
        new cpp_bitfield(std::move(name), std::move(type), no_bits, is_mutable));
    idx.register_definition(std::move(id), type_safe::cref(*result));
    return result;
}

std::unique_ptr<cpp_bitfield> cpp_bitfield::build(std::unique_ptr<cpp_type> type, unsigned no_bits,
                                                  bool is_mutable)
{
    return std::unique_ptr<cpp_bitfield>(
        new cpp_bitfield("", std::move(type), no_bits, is_mutable));
}

cpp_entity_kind cpp_bitfield::do_get_entity_kind() const noexcept
{
    return kind();
}
