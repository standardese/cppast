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

            /// \effects Adds a parameter.
            void add_parameter(std::unique_ptr<cpp_template_parameter> parameter)
            {
                static_cast<cpp_template&>(*template_entity)
                    .parameters_.push_back(*template_entity, std::move(parameter));
            }

            /// \effects Registers the template.
            /// \returns The finished template.
            std::unique_ptr<T> finish(const cpp_entity_index& idx, cpp_entity_id id)
            {
                idx.register_entity(std::move(id), type_safe::cref(*template_entity));
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

    /// Base class for all entities modelling a C++ template specialization.
    class cpp_template_specialization : public cpp_template
    {
    public:
        /// \returns A reference to the template that is being specialized.
        const cpp_template& primary_template() const noexcept
        {
            return *templ_;
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
        template <class T, class EntityT, class TemplateT>
        class specialization_builder : public basic_builder<T, EntityT>
        {
        public:
            /// \effects Sets the entity that is being templated and the primary template.
            specialization_builder(std::unique_ptr<EntityT>               entity,
                                   type_safe::object_ref<const TemplateT> templ)
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
        cpp_template_specialization(std::unique_ptr<cpp_entity>               entity,
                                    type_safe::object_ref<const cpp_template> templ)
        : cpp_template(std::move(entity)), templ_(templ)
        {
        }

    private:
        std::vector<cpp_template_argument>        arguments_;
        type_safe::object_ref<const cpp_template> templ_;
    };
} // namespace cppast

#endif // CPPAST_CPP_TEMPLATE_HPP_INCLUDED
