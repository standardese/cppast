// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_FILE_HPP_INCLUDED
#define CPPAST_CPP_FILE_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>

namespace cppast
{
    /// A [cppast::cpp_entity]() modelling a file.
    ///
    /// This is the top-level entity of the AST.
    class cpp_file final : public cpp_entity
    {
    public:
        /// Builds a [cppast::cpp_file]().
        class builder
        {
        public:
            /// \effects Sets the file name.
            explicit builder(std::string name) : file_(new cpp_file(std::move(name)))
            {
            }

            /// \effects Adds an entity.
            void add_child(std::unique_ptr<cpp_entity> child) noexcept
            {
                file_->children_.push_back(std::move(child));
            }

            /// \returns The finished file.
            std::unique_ptr<cpp_file> finish() noexcept
            {
                return std::move(file_);
            }

            /// \returns A reference to (not yet finished) file.
            const cpp_file& get() const noexcept
            {
                return *file_;
            }

        private:
            std::unique_ptr<cpp_file> file_;
        };

        using iterator = detail::intrusive_list<cpp_entity>::const_iterator;

        /// \returns A const iterator to the first child.
        iterator begin() const noexcept
        {
            return children_.begin();
        }

        /// \returns A const iterator to the last child.
        iterator end() const noexcept
        {
            return children_.end();
        }

    private:
        cpp_file(std::string name) : cpp_entity(nullptr, std::move(name))
        {
        }

        /// \returns [cpp_entity_type::file_t]().
        cpp_entity_type do_get_entity_type() const noexcept override;

        detail::intrusive_list<cpp_entity> children_;
    };
} // namespace cppast

#endif // CPPAST_CPP_FILE_HPP_INCLUDED
