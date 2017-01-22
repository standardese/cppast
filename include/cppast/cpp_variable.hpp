// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_VARIABLE_HPP_INCLUDED
#define CPPAST_CPP_VARIABLE_HPP_INCLUDED

#include <cppast/cpp_variable_base.hpp>

namespace cppast
{
    /// Storage class and other specifiers for a [cppast::cpp_variable]().
    enum cpp_variable_specifiers
    {
        cpp_var_none = 0,

        cpp_var_static = 1,
        cpp_var_extern = 2,

        cpp_var_thread_local = 4,

        cpp_var_constexpr = 8,
    };

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
                                                   cpp_variable_specifiers         spec);

        /// \returns The [cppast::cpp_variable_specifiers]() on that variable.
        cpp_variable_specifiers specifiers() const noexcept
        {
            return specifiers_;
        }

    private:
        cpp_variable(std::string name, std::unique_ptr<cpp_type> type,
                     std::unique_ptr<cpp_expression> def, cpp_variable_specifiers spec)
        : cpp_variable_base(std::move(name), std::move(type), std::move(def)), specifiers_(spec)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        cpp_variable_specifiers specifiers_;
    };
} // namespace cppast

#endif // CPPAST_CPP_VARIABLE_HPP_INCLUDED
