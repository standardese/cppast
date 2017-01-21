// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_NAMESPACE_HPP_INCLUDED
#define CPPAST_CPP_NAMESPACE_HPP_INCLUDED

#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_scope.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling a namespace.
    class cpp_namespace final : public cpp_scope
    {
    public:
        /// Builds a [cppast::cpp_namespace]().
        class builder
        {
        public:
            /// \effects Sets the namespace name and whether it is inline.
            explicit builder(std::string name, bool is_inline)
            : namespace_(new cpp_namespace(std::move(name), is_inline))
            {
            }

            /// \effects Adds an entity.
            void add_child(std::unique_ptr<cpp_entity> child) noexcept
            {
                namespace_->add_child(std::move(child));
            }

            /// \effects Registers the file in the [cppast::cpp_entity_index](),
            /// using the given [cppast::cpp_entity_id]().
            /// \returns The finished namespace.
            std::unique_ptr<cpp_namespace> finish(const cpp_entity_index& idx,
                                                  cpp_entity_id           id) noexcept
            {
                idx.register_entity(std::move(id), type_safe::ref(*namespace_));
                return std::move(namespace_);
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
        cpp_namespace(std::string name, bool is_inline)
        : cpp_scope(std::move(name)), inline_(is_inline)
        {
        }

        cpp_entity_type do_get_entity_type() const noexcept override;

        bool inline_;
    };
} // namespace cppast

#endif // CPPAST_CPP_NAMESPACE_HPP_INCLUDED
