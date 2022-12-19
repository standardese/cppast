// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CPP_TEMPLATE_PARAMETER_HPP_INCLUDED
#define CPPAST_CPP_TEMPLATE_PARAMETER_HPP_INCLUDED

#include <type_safe/optional.hpp>
#include <type_safe/variant.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_variable_base.hpp>
#include <cppast/detail/intrusive_list.hpp>

namespace cppast
{
/// Base class for all entities modelling a template parameter of some kind.
class cpp_template_parameter : public cpp_entity
{
public:
    /// \returns Whether or not the parameter is variadic.
    bool is_variadic() const noexcept
    {
        return variadic_;
    }

protected:
    cpp_template_parameter(std::string name, bool variadic)
    : cpp_entity(std::move(name)), variadic_(variadic)
    {}

private:
    bool variadic_;
};

/// The kind of keyword used in a template parameter.
enum class cpp_template_keyword
{
    keyword_class,
    keyword_typename,
    concept_contraint
};

/// \returns The string associated of the keyword.
const char* to_string(cpp_template_keyword kw) noexcept;

/// A [cppast::cpp_entity]() modelling a C++ template type parameter.
class cpp_template_type_parameter final : public cpp_template_parameter
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered template type parameter.
    /// \notes The `default_type` may be `nullptr` in which case the parameter has no default.
    static std::unique_ptr<cpp_template_type_parameter> build(
        const cpp_entity_index& idx, cpp_entity_id id, std::string name, cpp_template_keyword kw,
        bool variadic, std::unique_ptr<cpp_type> default_type = nullptr,
        type_safe::optional<cpp_token_string> concept_constraint = type_safe::nullopt);

    /// \returns A [ts::optional_ref]() to the default type.
    type_safe::optional_ref<const cpp_type> default_type() const noexcept
    {
        return type_safe::opt_cref(default_type_.get());
    }

    /// \returns The keyword used in the template parameter.
    cpp_template_keyword keyword() const noexcept
    {
        return keyword_;
    }

    const type_safe::optional<cpp_token_string>& concept_constraint() const noexcept
    {
        return concept_constraint_;
    }

private:
    cpp_template_type_parameter(std::string name, cpp_template_keyword kw, bool variadic,
                                std::unique_ptr<cpp_type>             default_type,
                                type_safe::optional<cpp_token_string> concept_constraint)
    : cpp_template_parameter(std::move(name), variadic), default_type_(std::move(default_type)),
      keyword_(kw), concept_constraint_(concept_constraint)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    std::unique_ptr<cpp_type> default_type_;
    cpp_template_keyword      keyword_;
    type_safe::optional<cpp_token_string> concept_constraint_;
};

/// \exclude
namespace detail
{
    struct cpp_template_parameter_ref_predicate
    {
        bool operator()(const cpp_entity& e);
    };
} // namespace detail

/// Reference to a [cppast::cpp_template_type_parameter]().
using cpp_template_type_parameter_ref
    = basic_cpp_entity_ref<cpp_template_type_parameter,
                           detail::cpp_template_parameter_ref_predicate>;

/// A [cppast::cpp_type]() defined by a [cppast::cpp_template_type_parameter]().
class cpp_template_parameter_type final : public cpp_type
{
public:
    /// \returns A newly created parameter type.
    static std::unique_ptr<cpp_template_parameter_type> build(
        cpp_template_type_parameter_ref parameter)
    {
        return std::unique_ptr<cpp_template_parameter_type>(
            new cpp_template_parameter_type(std::move(parameter)));
    }

    /// \returns A reference to the [cppast::cpp_template_type_parameter]() this type refers to.
    const cpp_template_type_parameter_ref& entity() const noexcept
    {
        return parameter_;
    }

private:
    cpp_template_parameter_type(cpp_template_type_parameter_ref parameter)
    : parameter_(std::move(parameter))
    {}

    cpp_type_kind do_get_kind() const noexcept override
    {
        return cpp_type_kind::template_parameter_t;
    }

    cpp_template_type_parameter_ref parameter_;
};

/// A [cppast::cpp_entity]() modelling a C++ non-type template parameter.
class cpp_non_type_template_parameter final : public cpp_template_parameter,
                                              public cpp_variable_base
{
public:
    static cpp_entity_kind kind() noexcept;

    /// \returns A newly created and registered non type template parameter.
    /// \notes The `default_value` may be `nullptr` in which case the parameter has no default.
    static std::unique_ptr<cpp_non_type_template_parameter> build(
        const cpp_entity_index& idx, cpp_entity_id id, std::string name,
        std::unique_ptr<cpp_type> type, bool is_variadic,
        std::unique_ptr<cpp_expression> default_value = nullptr);

private:
    cpp_non_type_template_parameter(std::string name, std::unique_ptr<cpp_type> type, bool variadic,
                                    std::unique_ptr<cpp_expression> def)
    : cpp_template_parameter(std::move(name), variadic),
      cpp_variable_base(std::move(type), std::move(def))
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;
};

/// \exclude
namespace detail
{
    struct cpp_template_ref_predicate
    {
        bool operator()(const cpp_entity& e);
    };
} // namespace detail

/// A reference to a [cppast::cpp_template]() or a [cppast::cpp_template_template_parameter]().
using cpp_template_ref = basic_cpp_entity_ref<cpp_entity, detail::cpp_template_ref_predicate>;

/// A [cppast::cpp_entity]() modelling a C++ template template parameter.
class cpp_template_template_parameter final : public cpp_template_parameter
{
public:
    static cpp_entity_kind kind() noexcept;

    /// Builds a [cppast::cpp_template_template_parameter]().
    class builder
    {
    public:
        /// \effects Sets the name and whether it is variadic.
        builder(std::string name, bool variadic)
        : parameter_(new cpp_template_template_parameter(std::move(name), variadic))
        {}

        /// \effects Sets the keyword,
        /// default is [cpp_template_keyword::keyword_class]().
        void keyword(cpp_template_keyword kw)
        {
            parameter_->keyword_ = kw;
        }

        /// \effects Adds a parameter to the template.
        void add_parameter(std::unique_ptr<cpp_template_parameter> param)
        {
            parameter_->parameters_.push_back(*parameter_, std::move(param));
        }

        /// \effects Sets the default template.
        void default_template(cpp_template_ref templ)
        {
            parameter_->default_ = std::move(templ);
        }

        /// \effects Registers the parameter in the [cppast::cpp_entity_index](),
        /// using the given [cppast::cpp_entity_id]().
        /// \returns The finished parameter.
        std::unique_ptr<cpp_template_template_parameter> finish(const cpp_entity_index& idx,
                                                                cpp_entity_id           id)
        {
            idx.register_definition(std::move(id), type_safe::ref(*parameter_));
            return std::move(parameter_);
        }

    private:
        std::unique_ptr<cpp_template_template_parameter> parameter_;
    };

    /// \returns An iteratable object containing the template parameters of the template template
    /// parameter.
    detail::iteratable_intrusive_list<cpp_template_parameter> parameters() const noexcept
    {
        return type_safe::ref(parameters_);
    }

    /// \returns The keyword used in the template parameter.
    cpp_template_keyword keyword() const noexcept
    {
        return keyword_;
    }

    /// \returns A [ts::optional]() that is the default template.
    type_safe::optional<cpp_template_ref> default_template() const noexcept
    {
        return default_;
    }

private:
    cpp_template_template_parameter(std::string name, bool variadic)
    : cpp_template_parameter(std::move(name), variadic),
      keyword_(cpp_template_keyword::keyword_class)
    {}

    cpp_entity_kind do_get_entity_kind() const noexcept override;

    detail::intrusive_list<cpp_template_parameter> parameters_;
    type_safe::optional<cpp_template_ref>          default_;
    cpp_template_keyword                           keyword_;
};

/// An argument for a [cppast::cpp_template_parameter]().
///
/// It is based on a [ts::variant]() of [cppast::cpp_type]() (for
/// [cppast::cpp_template_type_parameter]()), [cppast::cpp_expression]() (for
/// [cppast::cpp_non_type_template_parameter]()) and [cppast::cpp_template_ref]() (for
/// [cppast::cpp_template_template_parameter]().
class cpp_template_argument
{
public:
    /// \effects Initializes it passing a type as argument.
    /// This corresponds to a [cppast::cpp_template_type_parameter]().
    /// \notes This constructor only participates in overload resolution if `T` is dervied from
    /// [cppast::cpp_type](). \param 1 \exclude
    template <typename T,
              typename std::enable_if<std::is_base_of<cpp_type, T>::value, int>::type = 0>
    cpp_template_argument(std::unique_ptr<T> type)
    : arg_(std::unique_ptr<cpp_type>(std::move(type)))
    {}

    /// \effects Initializes it passing an expression as argument.
    /// This corresponds to a [cppast::cpp_non_type_template_parameter]().
    /// \notes This constructor only participates in overload resolution if `T` is dervied from
    /// [cppast::cpp_expression](). \param 1 \exclude
    template <typename T,
              typename = typename std::enable_if<std::is_base_of<cpp_expression, T>::value>::type>
    cpp_template_argument(std::unique_ptr<T> expr)
    : arg_(std::unique_ptr<cpp_expression>(std::move(expr)))
    {}

    /// \effects Initializes it passing a template as argument.
    /// This corresponds to a [cppast::cpp_template_template_parameter]().
    cpp_template_argument(cpp_template_ref templ) : arg_(std::move(templ)) {}

    type_safe::optional_ref<const cpp_type> type() const noexcept
    {
        return arg_.optional_value(type_safe::variant_type<std::unique_ptr<cpp_type>>{})
            .map([](const std::unique_ptr<cpp_type>& type) { return type_safe::ref(*type); });
    }

    type_safe::optional_ref<const cpp_expression> expression() const noexcept
    {
        return arg_.optional_value(type_safe::variant_type<std::unique_ptr<cpp_expression>>{})
            .map([](const std::unique_ptr<cpp_expression>& expr) { return type_safe::ref(*expr); });
    }

    type_safe::optional_ref<const cpp_template_ref> template_ref() const noexcept
    {
        return arg_.optional_value(type_safe::variant_type<cpp_template_ref>{});
    }

private:
    type_safe::variant<std::unique_ptr<cpp_type>, std::unique_ptr<cpp_expression>, cpp_template_ref>
        arg_;
};
} // namespace cppast

#endif // CPPAST_CPP_TEMPLATE_PARAMETER_HPP_INCLUDED
