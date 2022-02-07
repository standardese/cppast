// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_FILE_HPP_INCLUDED
#define CPPAST_CPP_FILE_HPP_INCLUDED

#include <vector>

#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity_ref.hpp>

namespace cppast
{
/// An unmatched documentation comment.
struct cpp_doc_comment
{
    std::string content;
    unsigned    line;

    cpp_doc_comment(std::string content, unsigned line) : content(std::move(content)), line(line) {}
};

/// A [cppast::cpp_entity]() modelling a file.
///
/// This is the top-level entity of the AST.
class cpp_file final : public cpp_entity, public cpp_entity_container<cpp_file, cpp_entity>
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builds a [cppast::cpp_file]().
    class builder
    {
    public:
        /// \effects Sets the file name.
        explicit builder(std::string name) : file_(new cpp_file(std::move(name))) {}

        /// \effects Adds an entity.
        void add_child(std::unique_ptr<cpp_entity> child) noexcept
        {
            file_->add_child(std::move(child));
        }

        /// \effects Adds an unmatched documentation comment.
        void add_unmatched_comment(cpp_doc_comment comment)
        {
            file_->comments_.push_back(std::move(comment));
        }

        /// \returns The not yet finished file.
        cpp_file& get() noexcept
        {
            return *file_;
        }

        /// \effects Registers the file in the [cppast::cpp_entity_index]().
        /// It will use the file name as identifier.
        /// \returns The finished file, or `nullptr`, if that file was already registered.
        std::unique_ptr<cpp_file> finish(const cpp_entity_index& idx) noexcept
        {
            auto res = idx.register_file(cpp_entity_id(file_->name()), type_safe::ref(*file_));
            return res ? std::move(file_) : nullptr;
        }

    private:
        std::unique_ptr<cpp_file> file_;
    };

    /// \returns The unmatched documentation comments.
    type_safe::array_ref<const cpp_doc_comment> unmatched_comments() const noexcept
    {
        return type_safe::ref(comments_.data(), comments_.size());
    }

private:
    cpp_file(std::string name) : cpp_entity(std::move(name)) {}

    /// \returns [cpp_entity_type::file_t]().
    cpp_entity_kind do_get_entity_kind() const noexcept override;

    std::vector<cpp_doc_comment> comments_;
};

/// \exclude
namespace detail
{
    struct cpp_file_ref_predicate
    {
        bool operator()(const cpp_entity& e);
    };
} // namespace detail

/// A reference to a [cppast::cpp_file]().
using cpp_file_ref = basic_cpp_entity_ref<cpp_file, detail::cpp_file_ref_predicate>;
} // namespace cppast

#endif // CPPAST_CPP_FILE_HPP_INCLUDED
