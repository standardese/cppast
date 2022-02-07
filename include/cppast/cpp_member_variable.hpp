// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_MEMBER_VARIABLE_HPP_INCLUDED
#define CPPAST_CPP_MEMBER_VARIABLE_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_variable_base.hpp>

namespace cppast
{
/// Base class for all kinds of member variables.
class cpp_member_variable_base : public cpp_entity, public cpp_variable_base
{
public:
    /// \returns Whether or not the member variable is declared `mutable`.
    bool is_mutable() const noexcept
    {
        return mutable_;
    }

    cpp_member_variable_base(std::string name, std::unique_ptr<cpp_type> type,
                             std::unique_ptr<cpp_expression> def, bool is_mutable)
    : cpp_entity(std::move(name)), cpp_variable_base(std::move(type), std::move(def)),
      mutable_(is_mutable)
    {}

private:
    bool mutable_;
};

/// A [cppast::cpp_entity]() modelling a C++ member variable.
class cpp_member_variable final : public cpp_member_variable_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered member variable.
    /// \notes `def` may be `nullptr` in which case there is no member initializer provided.
    static std::unique_ptr<cpp_member_variable> build(const cpp_entity_index& idx, cpp_entity_id id,
                                                      std::string                     name,
                                                      std::unique_ptr<cpp_type>       type,
                                                      std::unique_ptr<cpp_expression> def,
                                                      bool                            is_mutable);

private:
    using cpp_member_variable_base::cpp_member_variable_base;

    cpp_entity_kind do_get_entity_kind() const noexcept override;
};

/// A [cppast::cpp_entity]() modelling a C++ bitfield.
class cpp_bitfield final : public cpp_member_variable_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered bitfield.
    /// \notes It cannot have a member initializer, i.e. default value.
    static std::unique_ptr<cpp_bitfield> build(const cpp_entity_index& idx, cpp_entity_id id,
                                               std::string name, std::unique_ptr<cpp_type> type,
                                               unsigned no_bits, bool is_mutable);

    /// \returns A newly created unnamed bitfield.
    /// \notes It will not be registered, as it is unnamed.
    static std::unique_ptr<cpp_bitfield> build(std::unique_ptr<cpp_type> type, unsigned no_bits,
                                               bool is_mutable);

    /// \returns The number of bits of the bitfield.
    unsigned no_bits() const noexcept
    {
        return bits_;
    }

private:
    cpp_bitfield(std::string name, std::unique_ptr<cpp_type> type, unsigned no_bits,
                 bool is_mutable)
    : cpp_member_variable_base(std::move(name), std::move(type), nullptr, is_mutable),
      bits_(no_bits)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    unsigned bits_;
};
} // namespace cppast

#endif // CPPAST_CPP_MEMBER_VARIABLE_HPP_INCLUDED
