// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_STORAGE_SPECIFIERS_HPP_INCLUDED
#define CPPAST_CPP_STORAGE_SPECIFIERS_HPP_INCLUDED

namespace cppast
{
    /// C++ storage class specifiers.
    enum cpp_storage_specifiers
    {
        cpp_storage_class_none = 0,

        cpp_storage_class_static = 1,
        cpp_storage_class_extern = 2,

        cpp_storage_class_thread_local = 4,
    };

    /// \returns Whether the [cppast::cpp_storage_specifiers]() contain `thread_local`.
    inline bool is_thread_local(cpp_storage_specifiers spec) noexcept
    {
        return (spec & cpp_storage_class_thread_local) != 0;
    }

    /// \returns Whether the [cppast::cpp_storage_specifiers]() contain `static`.
    inline bool is_static(cpp_storage_specifiers spec) noexcept
    {
        return (spec & cpp_storage_class_static) != 0;
    }

    /// \returns Whether the [cppast::cpp_storage_specifiers]() contain `extern`.
    inline bool is_extern(cpp_storage_specifiers spec) noexcept
    {
        return (spec & cpp_storage_class_extern) != 0;
    }
} // namespace cppast

#endif // CPPAST_CPP_STORAGE_SPECIFIERS_HPP_INCLUDED
