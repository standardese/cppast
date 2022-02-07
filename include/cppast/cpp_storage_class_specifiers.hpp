// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_STORAGE_CLASS_SPECIFIERS_HPP_INCLUDED
#define CPPAST_CPP_STORAGE_CLASS_SPECIFIERS_HPP_INCLUDED

#include <cppast/cppast_fwd.hpp>

namespace cppast
{
/// C++ storage class specifiers.
///
/// See http://en.cppreference.com/w/cpp/language/storage_duration, for example.
/// \notes These are just all the possible *keywords* used in a variable declaration,
/// not necessarily their *semantic* meaning.
enum cpp_storage_class_specifiers : int
{
    cpp_storage_class_none = 0, //< no storage class specifier given.

    cpp_storage_class_auto = 1, //< *automatic* storage duration.

    cpp_storage_class_static = 2, //< *static* or *thread* storage duration and *internal* linkage.
    cpp_storage_class_extern = 4, //< *static* or *thread* storage duration and *external* linkage.

    cpp_storage_class_thread_local = 8, //< *thread* storage duration.
    /// \notes This is the only one that can be combined with the others.
};

/// \returns Whether the [cppast::cpp_storage_class_specifiers]() contain `thread_local`.
inline bool is_thread_local(cpp_storage_class_specifiers spec) noexcept
{
    return (spec & cpp_storage_class_thread_local) != 0;
}

/// \returns Whether the [cppast::cpp_storage_class_speicifers]() contain `auto`.
inline bool is_auto(cpp_storage_class_specifiers spec) noexcept
{
    return (spec & cpp_storage_class_auto) != 0;
}

/// \returns Whether the [cppast::cpp_storage_class_specifiers]() contain `static`.
inline bool is_static(cpp_storage_class_specifiers spec) noexcept
{
    return (spec & cpp_storage_class_static) != 0;
}

/// \returns Whether the [cppast::cpp_storage_class_specifiers]() contain `extern`.
inline bool is_extern(cpp_storage_class_specifiers spec) noexcept
{
    return (spec & cpp_storage_class_extern) != 0;
}
} // namespace cppast

#endif // CPPAST_CPP_STORAGE_CLASS_SPECIFIERS_HPP_INCLUDED
