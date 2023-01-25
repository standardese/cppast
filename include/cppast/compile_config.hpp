// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_COMPILE_CONFIG_HPP_INCLUDED
#define CPPAST_COMPILE_CONFIG_HPP_INCLUDED

#include <string>
#include <vector>

#include <type_safe/flag_set.hpp>
#include <type_safe/reference.hpp>

#include <cppast/cppast_fwd.hpp>
#include <cppast/detail/assert.hpp>

namespace cppast
{
/// The C/C++ standard that should be used.
enum class cpp_standard
{
    cpp_98,
    cpp_03,
    cpp_11,
    cpp_14,
    cpp_1z,
    cpp_17,
    cpp_2a,
    cpp_20,
    cpp_2b,

    c_89,
    c_99,
    c_11,
    c_17,
    c_2x,

    cpp_latest = cpp_standard::cpp_14, //< The latest supported C++ standard.
    c_latest = cpp_standard::c_17, //< The latest supported C standard.
};

/// \returns A human readable string representing the option,
/// it is e.g. `c++14` for `cpp_14`.
inline const char* to_string(cpp_standard standard) noexcept
{
    switch (standard)
    {
    case cpp_standard::cpp_98:
        return "c++98";
    case cpp_standard::cpp_03:
        return "c++03";
    case cpp_standard::cpp_11:
        return "c++11";
    case cpp_standard::cpp_14:
        return "c++14";
    case cpp_standard::cpp_1z:
        return "c++1z";
    case cpp_standard::cpp_17:
        return "c++17";
    case cpp_standard::cpp_2a:
        return "c++2a";
    case cpp_standard::cpp_20:
        return "c++20";
    case cpp_standard::cpp_2b:
        return "c++2b";

    case cpp_standard::c_89:
        return "c89";
    case cpp_standard::c_99:
        return "c99";
    case cpp_standard::c_11:
        return "c11";
    case cpp_standard::c_17:
        return "c17";
    case cpp_standard::c_2x:
        return "c2x";
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return "ups";
}

/// \returns whether the language standard is a C standard
inline bool is_c_standard(cpp_standard standard) noexcept
{
    switch (standard)
    {
    case cpp_standard::cpp_98:
    case cpp_standard::cpp_03:
    case cpp_standard::cpp_11:
    case cpp_standard::cpp_14:
    case cpp_standard::cpp_1z:
    case cpp_standard::cpp_17:
    case cpp_standard::cpp_2a:
    case cpp_standard::cpp_20:
    case cpp_standard::cpp_2b:
        return false;
    case cpp_standard::c_89:
    case cpp_standard::c_99:
    case cpp_standard::c_11:
    case cpp_standard::c_17:
    case cpp_standard::c_2x:
        return true;
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return false;
}

/// Other special compilation flags.
enum class compile_flag
{
    gnu_extensions, //< Enable GCC extensions.

    ms_extensions,    //< Enable MSVC extensions.
    ms_compatibility, //< Enable MSVC compatibility.

    _flag_set_size, //< \exclude
};

/// A [ts::flag_set]() of [cppast::compile_flag]().
using compile_flags = type_safe::flag_set<compile_flag>;

/// Base class for the configuration of a [cppast::parser]().
class compile_config
{
public:
    /// \effects Sets the given C++ standard and compilation flags.
    void set_flags(cpp_standard standard, compile_flags flags = {})
    {
        do_set_flags(standard, flags);
    }

    /// \effects Enables an `-fXXX` flag.
    /// \returns Whether or not it was a known feature and enabled.
    bool enable_feature(std::string name)
    {
        return do_enable_feature(name);
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

    /// \returns A unique name of the configuration.
    /// \notes This allows detecting mismatches of configurations and parsers.
    const char* name() const noexcept
    {
        return do_get_name();
    }

    /// \returns Whether to parse files as C rather than C++.
    bool use_c() const noexcept
    {
        return do_use_c();
    }

protected:
    compile_config(std::vector<std::string> def_flags) : flags_(std::move(def_flags)) {}

    compile_config(const compile_config&) = default;
    compile_config& operator=(const compile_config&) = default;

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
    virtual void do_set_flags(cpp_standard standard, compile_flags flags) = 0;

    /// \effects Sets the given feature flags.
    /// \returns Whether or not it was a known feature and set.
    virtual bool do_enable_feature(std::string name)
    {
        (void)name;
        return false;
    }

    /// \effects Adds the given path to the set of include directories.
    virtual void do_add_include_dir(std::string path) = 0;

    /// \effects Defines the given macro.
    virtual void do_add_macro_definition(std::string name, std::string definition) = 0;

    /// \effects Undefines the given macro.
    virtual void do_remove_macro_definition(std::string name) = 0;

    /// \returns A unique name of the configuration.
    /// \notes This allows detecting mismatches of configurations and parsers.
    virtual const char* do_get_name() const noexcept = 0;

    /// \returns Whether to parse files as C rather than C++.
    virtual bool do_use_c() const noexcept = 0;

    std::vector<std::string> flags_;
};
} // namespace cppast

#endif // CPPAST_COMPILE_CONFIG_HPP_INCLUDED
