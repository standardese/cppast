// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_RAII_WRAPPER_HPP_INCLUDED
#define CPPAST_RAII_WRAPPER_HPP_INCLUDED

#include <type_traits>

#include <clang-c/Index.h>

#include <cppast/detail/assert.hpp>

namespace cppast
{
    namespace detail
    {
        template <typename T, class Deleter>
        class raii_wrapper : Deleter
        {
            static_assert(std::is_pointer<T>::value, "");

        public:
            raii_wrapper() noexcept : obj_(nullptr)
            {
            }

            raii_wrapper(T obj) noexcept : obj_(obj)
            {
                DEBUG_ASSERT(obj_, detail::assert_handler{});
            }

            raii_wrapper(raii_wrapper&& other) noexcept : obj_(other.obj_)
            {
                DEBUG_ASSERT(obj_, detail::assert_handler{});
                other.obj_ = nullptr;
            }

            ~raii_wrapper() noexcept
            {
                if (obj_)
                    static_cast<Deleter&> (*this)(obj_);
            }

            raii_wrapper& operator=(raii_wrapper&& other) noexcept
            {
                raii_wrapper tmp(std::move(other));
                swap(*this, tmp);
                return *this;
            }

            friend void swap(raii_wrapper& a, raii_wrapper& b) noexcept
            {
                std::swap(a.obj_, b.obj_);
            }

            T get() const noexcept
            {
                DEBUG_ASSERT(obj_, detail::assert_handler{});
                return obj_;
            }

        private:
            T obj_;
        };

        struct cxindex_deleter
        {
            void operator()(CXIndex idx) noexcept
            {
                clang_disposeIndex(idx);
            }
        };

        using cxindex = raii_wrapper<CXIndex, cxindex_deleter>;

        struct cxtranslation_unit_deleter
        {
            void operator()(CXTranslationUnit unit) noexcept
            {
                clang_disposeTranslationUnit(unit);
            }
        };

        using cxtranslation_unit = raii_wrapper<CXTranslationUnit, cxtranslation_unit_deleter>;
    }
} // namespace cppast::detail

#endif // CPPAST_RAII_WRAPPER_HPP_INCLUDED
