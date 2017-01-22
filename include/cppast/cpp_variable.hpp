// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_VARIABLE_HPP_INCLUDED
#define CPPAST_CPP_VARIABLE_HPP_INCLUDED

#include <cppast/cpp_storage_specifiers.hpp>
#include <cppast/cpp_variable_base.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling a C++ variable.
    /// \notes This is not a member variable,
    /// use [cppast::cpp_member_variable]() for that.
    class cpp_variable final : public cpp_variable_base
    {
    public:
        /// \returns A newly created and registered variable.
        /// \notes The default value may be `nullptr` indicating no default value.
        static std::unique_ptr<cpp_variable> build(const cpp_entity_index& idx, cpp_entity_id id,
                                                   std::string name, std::unique_ptr<cpp_type> type,
                                                   std::unique_ptr<cpp_expression> def,
                                                   cpp_storage_specifiers spec, bool is_constexpr);

        /// \returns The [cppast::cpp_storage_specifiers]() on that variable.
        cpp_storage_specifiers storage_class() const noexcept
        {
            return storage_;
        }

        /// \returns Whether the variable is marked `constexpr`.
        bool is_constexpr() const noexcept
        {
            return is_constexpr_;
        }

    private:
        cpp_variable(std::string name, std::unique_ptr<cpp_type> type,
                     std::unique_ptr<cpp_expression> def, cpp_storage_specifiers spec,
                     bool is_constexpr)
        : cpp_variable_base(std::move(name), std::move(type), std::move(def)),
          storage_(spec),
          is_constexpr_(is_constexpr)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        cpp_storage_specifiers storage_;
        bool                   is_constexpr_;
    };
} // namespace cppast

#endif // CPPAST_CPP_VARIABLE_HPP_INCLUDED
