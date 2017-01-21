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

    /// A reference to a [cppast::cpp_namespace]().
    class cpp_namespace_ref
    {
    public:
        /// \effects Creates it giving it the target id and name.
        /// \requires A [cppast::cpp_namespace]() must register in the [cppast::cpp_entity_index]() with that id.
        cpp_namespace_ref(cpp_entity_id target_id, std::string target_name)
        : target_(std::move(target_id)), name_(std::move(target_name))
        {
        }

        /// \returns The name of the reference, as spelled in the source code.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns The [cppast::cpp_entity_id]() of the namespace it refers to.
        const cpp_entity_id& id() const noexcept
        {
            return target_;
        }

        /// \returns The [cppast::cpp_namespace]() it refers to.
        const cpp_namespace& get(const cpp_entity_index& idx) const noexcept;

    private:
        cpp_entity_id target_;
        std::string   name_;
    };

    /// A [cppast::cpp_entity]() modelling a namespace alias.
    class cpp_namespace_alias final : public cpp_entity
    {
    public:
        /// \returns A newly created and registered namespace alias.
        static std::unique_ptr<cpp_namespace_alias> build(const cpp_entity_index& idx,
                                                          cpp_entity_id id, std::string name,
                                                          cpp_namespace_ref target)
        {
            auto ptr = std::unique_ptr<cpp_namespace_alias>(
                new cpp_namespace_alias(std::move(name), std::move(target)));
            idx.register_entity(std::move(id), type_safe::ref(*ptr));
            return ptr;
        }

        /// \returns The [cppast::cpp_namespace_ref]() to the aliased namespace.
        const cpp_namespace_ref& target() const noexcept
        {
            return target_;
        }

    private:
        cpp_namespace_alias(std::string name, cpp_namespace_ref target)
        : cpp_entity(std::move(name)), target_(std::move(target))
        {
        }

        cpp_entity_type do_get_entity_type() const noexcept override;

        cpp_namespace_ref target_;
    };

    /// A [cppast::cpp_entity]() modelling a using directive.
    ///
    /// A using directive is `using namespace std`, for example.
    class cpp_using_directive final : public cpp_entity
    {
    public:
        /// \returns A newly created using directive.
        /// \notes It is not meant to be registered at the [cppast::cpp_entity_index](),
        /// as nothing can refer to it.
        static std::unique_ptr<cpp_using_directive> build(const cpp_namespace_ref& target)
        {
            return std::unique_ptr<cpp_using_directive>(new cpp_using_directive(target));
        }

        /// \returns The [cppast::cpp_namespace_ref]() that is exposed.
        /// \notes The name of the reference is the same as the name of this entity.
        cpp_namespace_ref target() const
        {
            return {target_, name()};
        }

    private:
        cpp_using_directive(const cpp_namespace_ref& target)
        : cpp_entity(target.name()), target_(target.id())
        {
        }

        cpp_entity_type do_get_entity_type() const noexcept override;

        cpp_entity_id target_;
    };
} // namespace cppast

#endif // CPPAST_CPP_NAMESPACE_HPP_INCLUDED
