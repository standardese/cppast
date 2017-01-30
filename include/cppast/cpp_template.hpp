// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_TEMPLATE_HPP_INCLUDED
#define CPPAST_CPP_TEMPLATE_HPP_INCLUDED

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
        detail::iteratable_intrusive_list<cpp_template_parameter> parameter_types() const noexcept
        {
            return type_safe::ref(parameters_);
        }

    protected:
        /// Builder class for templates.
        ///
        /// Inherit from it to provide additional setter.
        template <typename T, typename EntityT>
        class basic_builder
        {
        public:
            /// \effects Sets the name and the entity that is begin templated.
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

        /// \effects Sets the name of the template and the entity to be templated.
        /// \notes It does not include the parameters.
        cpp_template(std::unique_ptr<cpp_entity> entity) : cpp_entity(entity->name())
        {
            add_child(std::move(entity));
        }

    private:
        detail::intrusive_list<cpp_template_parameter> parameters_;
    };
} // namespace cppast

#endif // CPPAST_CPP_TEMPLATE_HPP_INCLUDED
