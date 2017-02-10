// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_COMPILE_CONFIG_HPP_INCLUDED
#define CPPAST_COMPILE_CONFIG_HPP_INCLUDED

#include <string>
#include <vector>

#include <type_safe/reference.hpp>
#include <type_safe/flag_set.hpp>

namespace cppast
{
    /// The C++ standard that should be used.
    enum class cpp_standard
    {
        cpp_98,
        cpp_03,
        cpp_11,
        cpp_14,

        cpp_latest = cpp_standard::cpp_14,
    };

    /// Other special compilation flags.
    enum class compile_flag
    {
        gnu_extensions, //< Enable GCC extensions.

        ms_extensions,    //< Enable MSVC extensions.
        ms_compatibility, //< Enable MSVC compatibility.

        _count, //< \exclude
    };
} // namespace cppast

namespace type_safe
{
    /// Specialization of [ts::flag_set_traits]() to use [cppast::compile_flag]() with [ts::flag_set]().
    template <>
    struct flag_set_traits<cppast::compile_flag> : std::true_type
    {
        static constexpr std::size_t size() noexcept
        {
            return static_cast<std::size_t>(cppast::compile_flag::_count);
        }
    };
} // namespace type_safe

namespace cppast
{
    /// Base class for the configuration of a [cppast::parser]().
    class compile_config
    {
    public:
        /// \effects Sets the given C++ standard and compilation flags.
        void set_standard(cpp_standard standard, type_safe::flag_set<compile_flag> flags)
        {
            do_set_flags(standard, flags);
        }

        /// \effects Adds the given path to the set of include directories.
        void add_include_dir(std::string path)
        {
            do_add_include_dir(std::move(path));
        }

        /// \effects Defines the given macro.
        void define_macro(std::string name, std::string definition)
        {
            do_add_macro_definition(std::move(name), std::move(definition));
        }

        /// \effects Undefines the given macro.
        void undefine_macro(std::string name)
        {
            do_remove_macro_definition(std::move(name));
        }

    protected:
        compile_config(std::vector<std::string> def_flags) : flags_(std::move(def_flags))
        {
        }

        ~compile_config() noexcept = default;

        void add_flag(std::string flag)
        {
            flags_.push_back(std::move(flag));
        }

        const std::vector<std::string>& get_flags() const noexcept
        {
            return flags_;
        }

    private:
        /// \effects Sets the given C++ standard and compilation flags.
        virtual void do_set_flags(cpp_standard                      standard,
                                  type_safe::flag_set<compile_flag> flags) = 0;

        /// \effects Adds the given path to the set of include directories.
        virtual void do_add_include_dir(std::string path) = 0;

        /// \effects Defines the given macro.
        virtual void do_add_macro_definition(std::string name, std::string definition) = 0;

        /// \effects Undefines the given macro.
        virtual void do_remove_macro_definition(std::string name) = 0;

        std::vector<std::string> flags_;
    };
} // namespace cppast

#endif // CPPAST_COMPILE_CONFIG_HPP_INCLUDED
