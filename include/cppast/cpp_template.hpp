// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_TEMPLATE_HPP_INCLUDED
#define CPPAST_CPP_TEMPLATE_HPP_INCLUDED

#include <vector>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_container.hpp>
#include <cppast/cpp_template_parameter.hpp>

namespace cppast
{
    /// Base class for all entities modelling a C++ template of some kind.
    ///
    /// It is a container of a single [cppast::cpp_entity]() that is the entity being templated.
    class cpp_template : public cpp_entity, public cpp_entity_container<cpp_template, cpp_entity>
    {
    public:
        /// \returns An iteratable object iterating over the [cppast::cpp_template_parameter]() entities.
        /// \notes These may be empty for a full specialization.
        detail::iteratable_intrusive_list<cpp_template_parameter> parameters() const noexcept
        {
            return type_safe::ref(parameters_);
        }

    protected:
        /// Builder class for templates.
        ///
        /// Inherit from it to provide additional setter.
        template <class T, class EntityT>
        class basic_builder
        {
        public:
            /// \effects Sets the entity that is begin templated.
            basic_builder(std::unique_ptr<EntityT> templ) : template_entity(new T(std::move(templ)))
            {
            }

            basic_builder(basic_builder&&) = default;

            /// \effects Adds a parameter.
            void add_parameter(std::unique_ptr<cpp_template_parameter> parameter)
            {
                static_cast<cpp_template&>(*template_entity)
                    .parameters_.push_back(*template_entity, std::move(parameter));
            }

            /// \returns The not yet finished template.
            T& get() const noexcept
            {
                return *template_entity;
            }

            /// \effects Registers the template.
            /// \returns The finished template.
            std::unique_ptr<T> finish(const cpp_entity_index& idx, cpp_entity_id id)
            {
                idx.register_definition(std::move(id), type_safe::cref(*template_entity));
                return std::move(template_entity);
            }

        protected:
            basic_builder()           = default;
            ~basic_builder() noexcept = default;

            std::unique_ptr<T> template_entity;
        };

        /// \effects Sets the entity to be templated.
        cpp_template(std::unique_ptr<cpp_entity> entity) : cpp_entity(entity->name())
        {
            add_child(std::move(entity));
        }

    private:
        detail::intrusive_list<cpp_template_parameter> parameters_;
    };

    /// A [cppast::cpp_type]() representing an instantiation of a [cppast::cpp_template]().
    class cpp_template_instantiation_type final : public cpp_type
    {
    public:
        /// Builds a [cppast::cpp_template_instantiation]().
        class builder
        {
        public:
            /// \effects Sets the primary template being instantiated.
            builder(cpp_template_ref templ)
            : result_(new cpp_template_instantiation_type(std::move(templ)))
            {
            }

            /// \effects Adds the next argument.
            void add_argument(cpp_template_argument arg)
            {
                result_->arguments_.push_back(std::move(arg));
            }

            /// \returns The finished instantiation.
            std::unique_ptr<cpp_template_instantiation_type> finish()
            {
                return std::move(result_);
            }

        private:
            std::unique_ptr<cpp_template_instantiation_type> result_;
        };

        /// \returns A reference to the template that is being instantiated.
        /// \notes It could also point to a specialization,
        /// this is just the *primary* template.
        const cpp_template_ref& primary_template() const noexcept
        {
            return templ_;
        }

        /// \returns An iteratable object iterating over the [cppast::cpp_template_argument]()s.
        /// \exclude return
        const std::vector<cpp_template_argument>& arguments() const noexcept
        {
            return arguments_;
        }

    private:
        cpp_template_instantiation_type(cpp_template_ref ref) : templ_(std::move(ref))
        {
        }

        cpp_type_kind do_get_kind() const noexcept override
        {
            return cpp_type_kind::template_instantiation;
        }

        std::vector<cpp_template_argument> arguments_;
        cpp_template_ref                   templ_;
    };

    /// Base class for all entities modelling a C++ template specialization.
    class cpp_template_specialization : public cpp_template
    {
    public:
        /// \returns A reference to the template that is being specialized.
        cpp_template_ref primary_template() const noexcept
        {
            return cpp_template_ref(templ_, name());
        }

        /// \returns An iteratable object iterating over the [cppast::cpp_template_argument]()s.
        /// \exclude return
        const std::vector<cpp_template_argument>& arguments() const noexcept
        {
            return arguments_;
        }

        /// \returns Whether or not the specialization is a full specialization.
        bool is_full_specialization() const noexcept
        {
            // if no template parameters are given, it is a full specialization
            return parameters().empty();
        }

    protected:
        /// Builder class for specializations.
        ///
        /// Inherit from it to provide additional setter.
        template <class T, class EntityT>
        class specialization_builder : public basic_builder<T, EntityT>
        {
        public:
            /// \effects Sets the entity that is being templated and the primary template.
            specialization_builder(std::unique_ptr<EntityT> entity, const cpp_template_ref& templ)
            {
                this->template_entity = std::unique_ptr<T>(new T(std::move(entity), templ));
            }

            /// \effects Adds the next argument for the [cppast::cpp_template_parameter]() of the primary template.
            void add_argument(cpp_template_argument arg)
            {
                auto specialization =
                    static_cast<const cpp_template_specialization&>(*this->template_entity);
                specialization.arguments_.push_back(std::move(arg));
            }

        protected:
            specialization_builder() = default;
        };

        /// \effects Sets the entity that is being templated and the primary template.
        cpp_template_specialization(std::unique_ptr<cpp_entity> entity,
                                    const cpp_template_ref&     templ)
        : cpp_template(std::move(entity)), templ_(templ.id()[0u])
        {
            DEBUG_ASSERT(!templ.is_overloaded() && templ.name() == entity->name(),
                         detail::precondition_error_handler{}, "invalid name of template ref");
        }

    private:
        std::vector<cpp_template_argument> arguments_;
        cpp_entity_id                      templ_;
    };
} // namespace cppast

#endif // CPPAST_CPP_TEMPLATE_HPP_INCLUDED
