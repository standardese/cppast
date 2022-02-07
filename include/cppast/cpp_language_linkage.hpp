// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_LANGUAGE_LINKAGE_HPP_INCLUDED
#define CPPAST_CPP_LANGUAGE_LINKAGE_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_container.hpp>

namespace cppast
{
/// A [cppast::cpp_entity]() modelling a language linkage.
class cpp_language_linkage final : public cpp_entity,
                                   public cpp_entity_container<cpp_language_linkage, cpp_entity>
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builds a [cppast::cpp_language_linkage]().
    class builder
    {
    public:
        /// \effects Sets the name, that is the kind of language linkage.
        explicit builder(std::string name) : linkage_(new cpp_language_linkage(std::move(name))) {}

        /// \effects Adds an entity to the language linkage.
        void add_child(std::unique_ptr<cpp_entity> child)
        {
            linkage_->add_child(std::move(child));
        }

        /// \returns The not yet finished language linkage.
        cpp_language_linkage& get() const noexcept
        {
            return *linkage_;
        }

        /// \returns The finalized language linkage.
        /// \notes It is not registered on purpose as nothing can refer to it.
        std::unique_ptr<cpp_language_linkage> finish()
        {
            return std::move(linkage_);
        }

    private:
        std::unique_ptr<cpp_language_linkage> linkage_;
    };

    /// \returns `true` if the linkage is a block, `false` otherwise.
    bool is_block() const noexcept;

private:
    using cpp_entity::cpp_entity;

    cpp_entity_kind do_get_entity_kind() const noexcept override;
};
} // namespace cppast

#endif // CPPAST_CPP_LANGUAGE_LINKAGE_HPP_INCLUDED
