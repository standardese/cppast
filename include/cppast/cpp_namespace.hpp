// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_NAMESPACE_HPP_INCLUDED
#define CPPAST_CPP_NAMESPACE_HPP_INCLUDED

#include <cppast/cpp_scope.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling a namespace.
    class cpp_namespace : public cpp_scope
    {
    public:
        /// Builds a [cppast::cpp_namespace]().
        class builder
        {
        public:
            /// \effects Sets the namespace name and whether it is inline.
            explicit builder(const cpp_entity& parent, std::string name, bool is_inline)
            : namespace_(new cpp_namespace(parent, std::move(name), is_inline))
            {
            }

            /// \effects Adds an entity.
            void add_child(std::unique_ptr<cpp_entity> child) noexcept
            {
                namespace_->add_child(std::move(child));
            }

            /// \returns The finished namespace.
            std::unique_ptr<cpp_namespace> finish() noexcept
            {
                return std::move(namespace_);
            }

            /// \returns A reference to (not yet finished) namespace.
            const cpp_namespace& get() const noexcept
            {
                return *namespace_;
            }

        private:
            std::unique_ptr<cpp_namespace> namespace_;
        };

        /// \returns Whether or not the namespace is an `inline namespace`.
        bool is_inline() const noexcept
        {
            return inline_;
        }

    private:
        cpp_namespace(const cpp_entity& parent, std::string name, bool is_inline)
        : cpp_scope(parent, std::move(name)), inline_(is_inline)
        {
        }

        cpp_entity_type do_get_entity_type() const noexcept override;

        bool inline_;
    };
} // namespace cppast

#endif // CPPAST_CPP_NAMESPACE_HPP_INCLUDED
