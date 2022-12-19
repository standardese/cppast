// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
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
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns the template parameters as a string
    const cpp_token_string& parameters() const noexcept
    {
        return parameters_;
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

        cpp_token_string& set_parameters(cpp_token_string string) noexcept
        {
            concept_->parameters_ = std::move(string);
            return concept_->parameters_;
        }

        cpp_expression& set_expression(std::unique_ptr<cpp_expression> expression) noexcept
        {
            concept_->expression_ = std::move(expression);
            return *concept_->expression_;
        }

        std::unique_ptr<cpp_concept> finish(const cpp_entity_index& idx, cpp_entity_id id);

    private:
        std::unique_ptr<cpp_concept> concept_;
    };

private:
    cpp_concept(std::string name) 
        : cpp_entity(std::move(name)), parameters_({})
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    cpp_token_string parameters_;

    std::unique_ptr<cpp_expression> expression_;
};


} // namespace cppast

#endif