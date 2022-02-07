// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_TYPE_ALIAS_HPP_INCLUDED
#define CPPAST_CPP_TYPE_ALIAS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_type.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a type alias/typedef.
/// \notes There is no distinction between `using` and `typedef` type aliases made in the AST.
class cpp_type_alias final : public cpp_entity
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered type alias.
    static std::unique_ptr<cpp_type_alias> build(const cpp_entity_index& idx, cpp_entity_id id,
                                                 std::string name, std::unique_ptr<cpp_type> type);

    /// \returns A newly created type alias that isn't registered.
    /// \notes This function is intendend for templated type aliases.
    static std::unique_ptr<cpp_type_alias> build(std::string name, std::unique_ptr<cpp_type> type);

    /// \returns A reference to the aliased [cppast::cpp_type]().
    const cpp_type& underlying_type() const noexcept
    {
        return *type_;
    }

private:
    cpp_type_alias(std::string name, std::unique_ptr<cpp_type> type)
    : cpp_entity(std::move(name)), type_(std::move(type))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    std::unique_ptr<cpp_type> type_;
};
} // namespace cppast

#endif // CPPAST_CPP_TYPE_ALIAS_HPP_INCLUDED
