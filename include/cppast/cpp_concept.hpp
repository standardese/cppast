// Copyright (C) 2017-2022 Jonathan M�ller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_CONCEPT_HPP_INCLUDED
#define CPPAST_CPP_CONCEPT_HPP_INCLUDED

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_template_parameter.hpp>
#include <cppast/cpp_expression.hpp>

namespace cppast
{
/// A [[cppast::cpp_entity]() modelling a c++ concept declaration
/// \notes while concepts are technically templates,
/// this is not a [cppast::cpp_template](),
/// as concepts act very differently from other templates
class cpp_concept final : public cpp_entity
{
    /// \returns An iteratable object iterating over the [cppast::cpp_template_parameter]()
    /// entities.
    detail::iteratable_intrusive_list<cpp_template_parameter> parameters() const noexcept
    {
        return type_safe::ref(parameters_);
    }

    /// \returns the [cppast::cpp_expression]() defining the concept constraint
    const cpp_expression& constraint_expression() const noexcept
    {
        return *expression_;
    }

    class builder
    {
    public:
        builder(std::string name)
        : concept_(new cpp_concept(std::move(name)))
        {}

        cpp_template_parameter& add_parameter(std::unique_ptr<cpp_template_parameter> param) noexcept
        {
            concept_->parameters_.push_back(*concept_, std::move(param));
        }

        cpp_expression& setExpression(std::unique_ptr<cpp_expression> expression) noexcept
        {
            concept_->expression_ = std::move(expression);
        }

        std::unique_ptr<cpp_concept> finish(const cpp_entity_index& idx, cpp_entity_id id);

    private:
        std::unique_ptr<cpp_concept> concept_;
    };

private:
    cpp_concept(std::string name) 
        : cpp_entity(std::move(name))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    detail::intrusive_list<cpp_template_parameter> parameters_;

    std::unique_ptr<cpp_expression> expression_;
};


} // namespace cppast

#endif