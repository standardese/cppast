// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_CLASS_HPP_INCLUDED
#define CPPAST_CPP_CLASS_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_container.hpp>

namespace cppast
{
    /// The keyword used on the declaration of a [cppast::cpp_class]().
    enum class cpp_class_kind
    {
        class_t,
        struct_t,
        union_t,
    };

    /// \returns The keyword as a string.
    const char* to_string(cpp_class_kind kind) noexcept;

    /// The C++ access specifiers.
    enum cpp_access_specifier_kind
    {
        cpp_public,
        cpp_protected,
        cpp_private
    };

    /// \returns The access specifier keyword as a string.
    const char* to_string(cpp_access_specifier_kind access) noexcept;

    /// A [cppast::cpp_entity]() modelling a C++ access specifier.
    class cpp_access_specifier final : public cpp_entity
    {
    public:
        /// \returns A newly created access specifier.
        /// \notes It is not meant to be registered at the [cppast::cpp_entity_index](),
        /// as nothing can refer to it.
        static std::unique_ptr<cpp_access_specifier> build(cpp_access_specifier_kind kind)
        {
            return std::unique_ptr<cpp_access_specifier>(new cpp_access_specifier(kind));
        }

        /// \returns The kind of access specifier.
        cpp_access_specifier_kind access_specifier() const noexcept
        {
            return access_;
        }

    private:
        cpp_access_specifier(cpp_access_specifier_kind access)
        : cpp_entity(to_string(access)), access_(access)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        cpp_access_specifier_kind access_;
    };

    /// A [cppast::cpp_entity]() modelling a C++ class.
    class cpp_class final : public cpp_entity, public cpp_entity_container<cpp_class, cpp_entity>
    {
    public:
        /// Builds a [cppast::cpp_class]().
        class builder
        {
        public:
            /// \effects Sets the name and kind and whether it is `final`.
            explicit builder(std::string name, cpp_class_kind kind, bool is_final)
            : class_(new cpp_class(std::move(name), kind, is_final))
            {
            }

            /// \effects Builds a [cppast::cpp_access_specifier]() and adds it.
            void access_specifier(cpp_access_specifier_kind access)
            {
                add_child(cpp_access_specifier::build(access));
            }

            /// \effects Adds an entity.
            void add_child(std::unique_ptr<cpp_entity> child) noexcept
            {
                class_->add_child(std::move(child));
            }

            /// \effects Registers the class in the [cppast::cpp_entity_index](),
            /// using the given [cppast::cpp_entity_id]().
            /// \returns The finished class.
            std::unique_ptr<cpp_class> finish(const cpp_entity_index& idx,
                                              cpp_entity_id           id) noexcept;

        private:
            std::unique_ptr<cpp_class> class_;
        };

        /// \returns The keyword used in the declaration of the class.
        cpp_class_kind class_kind() const noexcept
        {
            return kind_;
        }

        /// \returns Whether or not the class was declared `final`.
        bool is_final() const noexcept
        {
            return final_;
        }

    private:
        cpp_class(std::string name, cpp_class_kind kind, bool final)
        : cpp_entity(std::move(name)), kind_(kind), final_(final)
        {
        }

        cpp_entity_kind do_get_entity_kind() const noexcept override;

        /// \returns The name of the namespace.
        type_safe::optional<std::string> do_get_scope_name() const override
        {
            return name();
        }

        cpp_class_kind kind_;
        bool           final_;
    };
} // namespace cppast

#endif // CPPAST_CPP_CLASS_HPP_INCLUDED
