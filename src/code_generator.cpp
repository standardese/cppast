// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/code_generator.hpp>

#include <cppast/cpp_alias_template.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_friend.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_language_linkage.hpp>
#include <cppast/cpp_member_function.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_preprocessor.hpp>
#include <cppast/cpp_static_assert.hpp>
#include <cppast/cpp_template_parameter.hpp>
#include <cppast/cpp_token.hpp>
#include <cppast/cpp_type_alias.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_variable_template.hpp>

using namespace cppast;

namespace
{
void opening_brace(const code_generator::output& output)
{
    if (output.formatting().is_set(formatting_flags::brace_nl))
        output << newl;
    else if (output.formatting().is_set(formatting_flags::brace_ws))
        output << whitespace;
    output << punctuation("{");
}

void comma(const code_generator::output& output)
{
    output << punctuation(",");
    if (output.formatting().is_set(formatting_flags::comma_ws))
        output << whitespace;
}

void bracket_ws(const code_generator::output& output)
{
    if (output.formatting().is_set(formatting_flags::bracket_ws))
        output << whitespace;
}

void operator_ws(const code_generator::output& output)
{
    if (output.formatting().is_set(formatting_flags::operator_ws))
        output << whitespace;
}

bool generate_code_impl(code_generator& generator, const cpp_entity& e,
                        cpp_access_specifier_kind cur_access);

template <class Container, typename Sep>
bool write_container(code_generator::output& output, const Container& cont, Sep s,
                     cpp_access_specifier_kind cur_access)
{
    auto need_sep = false;
    for (auto& child : cont)
    {
        auto is_excluded = output.options(child, cur_access).is_set(code_generator::exclude);
        if (!is_excluded)
        {
            if (need_sep)
                output << s;
            need_sep = generate_code_impl(*output.generator(), child, cur_access);
        }
    }
    return need_sep;
}

bool generate_file(code_generator& generator, const cpp_file& f,
                   cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(f), cur_access);
    if (output)
    {
        auto need_sep = write_container(output, f, newl, cur_access);
        if (!need_sep)
            // file empty, write newl
            output << newl;
        else
            output.container_end();
    }
    return static_cast<bool>(output);
}

bool generate_macro_parameter(code_generator& generator, const cpp_macro_parameter& param,
                              cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(param), cur_access);
    if (output)
        output << identifier(param.name());
    return static_cast<bool>(output);
}

bool generate_macro_definition(code_generator& generator, const cpp_macro_definition& def,
                               cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(def), cur_access);
    if (output)
    {
        output << preprocessor_token("#define") << whitespace << identifier(def.name());
        if (def.is_function_like())
        {
            output << preprocessor_token("(") << bracket_ws;
            auto need_sep = write_container(output, def.parameters(), comma, cpp_public);
            if (def.is_variadic())
            {
                if (need_sep)
                    output << comma;
                output << preprocessor_token("...");
            }
            output << bracket_ws << preprocessor_token(")");
        }
        if (!def.replacement().empty() && !output.options().is_set(code_generator::declaration))
            output << whitespace << preprocessor_token(def.replacement()) << newl;
        else
            output << newl;
    }
    return static_cast<bool>(output);
}

bool generate_include_directive(code_generator& generator, const cpp_include_directive& include,
                                cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(include), cur_access);
    if (output)
    {
        output << preprocessor_token("#include") << whitespace;
        if (include.include_kind() == cpp_include_kind::system)
            output << preprocessor_token("<");
        else
            output << preprocessor_token("\"");
        output << include.target();
        if (include.include_kind() == cpp_include_kind::system)
            output << preprocessor_token(">");
        else
            output << preprocessor_token("\"");
        output << newl;
    }
    return static_cast<bool>(output);
}

bool generate_language_linkage(code_generator& generator, const cpp_language_linkage& linkage,
                               cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(linkage), cur_access);
    if (output)
    {
        output << keyword("extern") << whitespace << string_literal(linkage.name());
        if (linkage.is_block())
        {
            output << opening_brace;
            output.indent();

            write_container(output, linkage, newl, cur_access);

            output.unindent();
            output << punctuation("}") << newl;
        }
        else
        {
            output << whitespace;
            generate_code_impl(generator, *linkage.begin(), cur_access);
        }
    }
    return static_cast<bool>(output);
}

bool generate_namespace(code_generator& generator, const cpp_namespace& ns,
                        cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(ns), cur_access);
    if (output)
    {
        if (ns.is_inline())
            output << keyword("inline") << whitespace;
        output << keyword("namespace") << whitespace << identifier(ns.name());
        output << opening_brace;
        output.indent();

        write_container(output, ns, newl, cur_access);
        output.container_end();

        output.unindent();
        output << punctuation("}") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_namespace_alias(code_generator& generator, const cpp_namespace_alias& alias,
                              cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(alias), cur_access);
    if (output)
    {
        output << keyword("namespace") << whitespace << identifier(alias.name()) << operator_ws
               << punctuation("=") << operator_ws;
        if (output.options() & code_generator::exclude_target)
            output.excluded(alias);
        else
            output << alias.target();
        output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_using_directive(code_generator& generator, const cpp_using_directive& directive,
                              cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(directive), cur_access);
    if (output)
        output << keyword("using") << whitespace << keyword("namespace") << whitespace
               << directive.target() << punctuation(";") << newl;
    return static_cast<bool>(output);
}

bool generate_using_declaration(code_generator& generator, const cpp_using_declaration& declaration,
                                cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(declaration),
                                  cur_access);
    if (output)
        output << keyword("using") << whitespace << declaration.target() << punctuation(";")
               << newl;
    return static_cast<bool>(output);
}

bool generate_type_alias(code_generator& generator, const cpp_type_alias& alias,
                         cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(alias), cur_access);
    if (output)
    {
        output << keyword("using") << whitespace << identifier(alias.name()) << operator_ws
               << punctuation("=") << operator_ws;
        if (output.options() & code_generator::exclude_target)
            output.excluded(alias);
        else
            detail::write_type(output, alias.underlying_type(), "");
        output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_enum_value(code_generator& generator, const cpp_enum_value& value,
                         cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(value), cur_access);
    if (output)
    {
        output << identifier(value.name());
        if (value.value())
        {
            output << operator_ws << punctuation("=") << operator_ws;
            detail::write_expression(output,
                                     value.value()
                                         .value()); // should have named something differently...
        }
    }
    return static_cast<bool>(output);
}

bool generate_enum(code_generator& generator, const cpp_enum& e,
                   cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(e), cur_access);
    if (output)
    {
        output << keyword("enum");
        if (e.is_scoped())
            output << whitespace << keyword("class");
        output << whitespace << identifier(e.semantic_scope()) << identifier(e.name());
        if (e.has_explicit_type())
        {
            output << newl << punctuation(":") << operator_ws;
            detail::write_type(output, e.underlying_type(), "");
        }

        if (output.generate_definition() && e.is_definition())
        {
            output << opening_brace;
            output.indent();

            auto need_sep = write_container(output, e,
                                            [](const code_generator::output& out) {
                                                out << punctuation(",") << newl;
                                            },
                                            cur_access);
            if (need_sep)
                output << newl;

            output.container_end();

            output.unindent();
            output << punctuation("};") << newl;
        }
        else
            output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

void write_access_specifier(code_generator::output& output, cpp_access_specifier_kind access)
{
    output.unindent();
    output << keyword(to_string(access)) << punctuation(":");
    output.indent();
}

bool generate_access_specifier(code_generator& generator, const cpp_access_specifier& access,
                               cpp_access_specifier_kind)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(access),
                                  access.access_specifier());
    if (output)
        write_access_specifier(output, access.access_specifier());
    return static_cast<bool>(output);
}

bool generate_base_class(code_generator& generator, const cpp_base_class& base,
                         cpp_access_specifier_kind)
{
    DEBUG_ASSERT(base.parent() && base.parent().value().kind() == cpp_entity_kind::class_t,
                 detail::assert_handler{});
    auto parent_kind = static_cast<const cpp_class&>(base.parent().value()).class_kind();

    code_generator::output output(type_safe::ref(generator), type_safe::ref(base),
                                  base.access_specifier());
    if (output)
    {
        if (base.is_virtual())
            output << keyword("virtual") << whitespace;

        auto access = base.access_specifier();
        if (access == cpp_protected)
            output << keyword("protected") << whitespace;
        else if (access == cpp_private && parent_kind != cpp_class_kind::class_t)
            output << keyword("private") << whitespace;
        else if (access == cpp_public && parent_kind == cpp_class_kind::class_t)
            output << keyword("public") << whitespace;

        output << identifier(base.name());
    }
    return static_cast<bool>(output);
}

void write_specialization_arguments(code_generator::output&            output,
                                    const cpp_template_specialization& spec)
{
    if (spec.arguments_exposed())
        detail::write_template_arguments(output, spec.arguments());
    else if (!spec.unexposed_arguments().empty())
    {
        output << punctuation("<") << bracket_ws;
        detail::write_token_string(output, spec.unexposed_arguments());
        output << bracket_ws << punctuation(">");
    }
}

void write_bases(code_generator& generator, code_generator::output& output, const cpp_class& c)
{
    auto need_sep = false;
    auto first    = true;
    for (auto& base : c.bases())
    {
        auto opt = output.options(base, base.access_specifier());
        if (first && !opt.is_set(code_generator::exclude))
        {
            first = false;
            output << newl << punctuation(":") << operator_ws;
        }
        else if (need_sep)
            output << comma;
        need_sep = generate_base_class(generator, base, cpp_public);
    }
}

bool generate_class(code_generator& generator, const cpp_class& c,
                    cpp_access_specifier_kind                                  cur_access,
                    type_safe::optional_ref<const cpp_template_specialization> spec = nullptr)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(c), cur_access);
    if (output)
    {
        if (is_friended(c))
            output << keyword("friend") << whitespace;
        output << keyword(to_string(c.class_kind())) << whitespace;

        output << identifier(c.semantic_scope());
        if (spec)
        {
            output << spec.value().primary_template();
            write_specialization_arguments(output, spec.value());
        }
        else
            output << identifier(c.name());

        if (c.is_final())
            output << whitespace << keyword("final");

        if (!output.generate_definition() || c.is_declaration())
            output << punctuation(";") << newl;
        else
        {
            write_bases(generator, output, c);
            output << opening_brace;
            output.indent();

            auto need_sep    = false;
            auto last_access = c.class_kind() == cpp_class_kind::class_t ? cpp_private : cpp_public;
            auto last_written_access = last_access;
            for (auto& member : c)
            {
                if (member.kind() == cpp_entity_kind::access_specifier_t)
                {
                    auto& access = static_cast<const cpp_access_specifier&>(member);
                    last_access  = access.access_specifier();
                }
                else if (output.options(member, last_access).is_set(code_generator::exclude))
                    continue;
                else
                {
                    if (need_sep)
                        output << newl;
                    if (last_access != last_written_access)
                    {
                        write_access_specifier(output, last_access);
                        last_written_access = last_access;
                    }
                    need_sep = generate_code_impl(generator, member, last_access);
                }
            }

            output.container_end();

            output.unindent();
            output << punctuation("};") << newl;
        }
    }
    return static_cast<bool>(output);
}

bool write_variable_base(code_generator::output& output, const cpp_variable_base& var,
                         const std::string& name)
{
    detail::write_type(output, var.type(), name);

    if (var.default_value())
    {
        output << operator_ws << punctuation("=") << operator_ws;
        detail::write_expression(output, var.default_value().value());
    }
    return static_cast<bool>(output);
}

void write_storage_class(code_generator::output& output, cpp_storage_class_specifiers storage,
                         bool is_constexpr, bool is_consteval)
{
    if (is_static(storage))
        output << keyword("static") << whitespace;
    if (is_extern(storage))
        output << keyword("extern") << whitespace;
    if (is_thread_local(storage))
        output << keyword("thread_local") << whitespace;
    if (is_constexpr)
        output << keyword("constexpr") << whitespace;
    else if (is_consteval)
        output << keyword("consteval") << whitespace;
}

bool generate_variable(code_generator& generator, const cpp_variable& var,
                       cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(var), cur_access);
    if (output)
    {
        write_storage_class(output, var.storage_class(), var.is_constexpr(), false);

        write_variable_base(output, var, var.name());
        output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_member_variable(code_generator& generator, const cpp_member_variable& var,
                              cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(var), cur_access);
    if (output)
    {
        if (var.is_mutable())
            output << keyword("mutable") << whitespace;
        write_variable_base(output, var, var.name());
        output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_bitfield(code_generator& generator, const cpp_bitfield& var,
                       cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(var), cur_access);
    if (output)
    {
        if (var.is_mutable())
            output << keyword("mutable") << whitespace;
        write_variable_base(output, var, var.name());
        output << operator_ws << punctuation(":") << operator_ws
               << int_literal(std::to_string(var.no_bits()));
        output << punctuation(";") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_function_parameter(code_generator& generator, const cpp_function_parameter& param,
                                 cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(param), cur_access);
    if (output)
        write_variable_base(output, param, param.name());
    return static_cast<bool>(output);
}

void write_function_parameters(code_generator::output& output, const cpp_function_base& base)
{
    output << punctuation("(") << bracket_ws;
    auto need_sep = write_container(output, base.parameters(), comma, cpp_public);
    if (base.is_variadic())
    {
        if (need_sep)
            output << comma;
        output << punctuation("...");
    }
    output << bracket_ws << punctuation(")");
}

void write_noexcept(code_generator::output& output, const cpp_function_base& base, bool need_ws)
{
    if (!base.noexcept_condition())
        return;
    else if (need_ws)
        output << whitespace;

    auto& cond = base.noexcept_condition().value();
    if (cond.kind() == cpp_expression_kind::literal_t
        && static_cast<const cpp_literal_expression&>(cond).value() == "true")
        output << keyword("noexcept");
    else
    {
        output << keyword("noexcept") << punctuation("(") << bracket_ws;
        // update check when expression gets exposed
        if (cond.kind() == cpp_expression_kind::unexposed_t
            && static_cast<const cpp_unexposed_expression&>(cond).expression().front().spelling
                   == "false")
            output << keyword("false");
        else if (output.options().is_set(code_generator::exclude_noexcept_condition))
            output.excluded(base);
        else
            detail::write_expression(output, cond);
        output << bracket_ws << punctuation(")");
    }
}

void write_function_body(code_generator::output& output, const cpp_function_base& base,
                         bool is_pure_virtual)
{
    switch (base.body_kind())
    {
    case cpp_function_declaration:
    case cpp_function_definition:
        if (is_pure_virtual)
            output << operator_ws << punctuation("=") << operator_ws << int_literal("0");
        output << punctuation(";") << newl;
        break;

    case cpp_function_defaulted:
        output << operator_ws << punctuation("=") << operator_ws << keyword("default")
               << punctuation(";") << newl;
        break;
    case cpp_function_deleted:
        output << operator_ws << punctuation("=") << operator_ws << keyword("delete")
               << punctuation(";") << newl;
        break;
    }
}

bool generate_function(code_generator& generator, const cpp_function& func,
                       cpp_access_specifier_kind                                  cur_access,
                       type_safe::optional_ref<const cpp_template_specialization> spec = nullptr)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(func), cur_access);
    if (output)
    {
        if (is_friended(func))
            output << keyword("friend") << whitespace;
        write_storage_class(output, func.storage_class(), func.is_constexpr(), func.is_consteval());

        if (output.options() & code_generator::exclude_return)
            output.excluded(func) << whitespace;
        else if (detail::is_complex_type(func.return_type()))
            output << keyword("auto") << whitespace;
        else
        {
            detail::write_type(output, func.return_type(), "");
            output << whitespace;
        }

        output << identifier(func.semantic_scope());
        if (spec)
        {
            output << spec.value().primary_template();
            write_specialization_arguments(output, spec.value());
        }
        else
            output << identifier(func.name());
        write_function_parameters(output, func);
        write_noexcept(output, func, output.formatting().is_set(formatting_flags::operator_ws));

        if (!(output.options() & code_generator::exclude_return)
            && detail::is_complex_type(func.return_type()))
        {
            output << operator_ws << punctuation("->") << operator_ws;
            detail::write_type(output, func.return_type(), "");
        }
        write_function_body(output, func, false);
    }
    return static_cast<bool>(output);
}

void write_prefix_virtual(code_generator::output& output, const cpp_virtual& virt)
{
    if (is_virtual(virt))
        output << keyword("virtual") << whitespace;
}

void write_suffix_virtual(code_generator::output& output, const cpp_virtual& virt,
                          bool is_definition)
{
    if (is_definition)
        // don't include it in definition
        return;

    if (is_overriding(virt))
        output << whitespace << keyword("override");
    if (is_final(virt))
        output << whitespace << keyword("final");
}

bool write_cv_ref(code_generator::output& output, const cpp_member_function_base& base)
{
    auto need_ws = false;
    switch (base.cv_qualifier())
    {
    case cpp_cv_none:
        break;
    case cpp_cv_const:
        output << operator_ws << keyword("const");
        need_ws = true;
        break;
    case cpp_cv_volatile:
        output << operator_ws << keyword("volatile");
        need_ws = true;
        break;
    case cpp_cv_const_volatile:
        output << operator_ws << keyword("const") << whitespace << keyword("volatile");
        need_ws = true;
        break;
    }

    switch (base.ref_qualifier())
    {
    case cpp_ref_none:
        break;
    case cpp_ref_lvalue:
        output << operator_ws << punctuation("&");
        need_ws = false;
        break;
    case cpp_ref_rvalue:
        output << operator_ws << punctuation("&&");
        need_ws = false;
        break;
    }

    return need_ws;
}

bool generate_member_function(code_generator& generator, const cpp_member_function& func,
                              cpp_access_specifier_kind                                  cur_access,
                              type_safe::optional_ref<const cpp_template_specialization> spec
                              = nullptr)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(func), cur_access);
    if (output)
    {
        if (is_friended(func))
            output << keyword("friend") << whitespace;
        if (func.is_constexpr())
            output << keyword("constexpr") << whitespace;
        else if (func.is_consteval())
            output << keyword("consteval") << whitespace;
        else
            write_prefix_virtual(output, func.virtual_info());

        if (output.options() & code_generator::exclude_return)
            output.excluded(func) << whitespace;
        else if (detail::is_complex_type(func.return_type()))
            output << keyword("auto") << whitespace;
        else
        {
            detail::write_type(output, func.return_type(), "");
            output << whitespace;
        }

        output << identifier(func.semantic_scope());
        if (spec)
        {
            output << spec.value().primary_template();
            write_specialization_arguments(output, spec.value());
        }
        else
            output << identifier(func.name());
        write_function_parameters(output, func);
        auto need_ws = write_cv_ref(output, func);
        write_noexcept(output, func,
                       need_ws || output.formatting().is_set(formatting_flags::operator_ws));

        if (!(output.options() & code_generator::exclude_return)
            && detail::is_complex_type(func.return_type()))
        {
            output << operator_ws << punctuation("->") << operator_ws;
            detail::write_type(output, func.return_type(), "");
        }

        write_suffix_virtual(output, func.virtual_info(), func.is_definition());
        write_function_body(output, func, is_pure(func.virtual_info()));
    }
    return static_cast<bool>(output);
}

bool generate_conversion_op(code_generator& generator, const cpp_conversion_op& op,
                            cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(op), cur_access);
    if (output)
    {
        if (is_friended(op))
            output << keyword("friend") << whitespace;
        if (op.is_explicit())
            output << keyword("explicit") << whitespace;
        if (op.is_constexpr())
            output << keyword("constexpr") << whitespace;
        else if (op.is_consteval())
            output << keyword("consteval") << whitespace;
        else
            write_prefix_virtual(output, op.virtual_info());

        output << identifier(op.semantic_scope());

        auto pos = op.name().find("operator");
        output << identifier(op.name().substr(0u, pos)) << keyword("operator") << whitespace;
        if (output.options() & code_generator::exclude_return)
            output.excluded(op);
        else
            detail::write_type(output, op.return_type(), "");

        output << punctuation("(") << punctuation(")");
        auto need_ws = write_cv_ref(output, op);
        write_noexcept(output, op,
                       need_ws || output.formatting().is_set(formatting_flags::operator_ws));

        write_suffix_virtual(output, op.virtual_info(), op.is_definition());
        write_function_body(output, op, is_pure(op.virtual_info()));
    }
    return static_cast<bool>(output);
}

bool generate_constructor(code_generator& generator, const cpp_constructor& ctor,
                          cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(ctor), cur_access);
    if (output)
    {
        if (is_friended(ctor))
            output << keyword("friend") << whitespace;
        if (ctor.is_explicit())
            output << keyword("explicit") << whitespace;
        if (ctor.is_constexpr())
            output << keyword("constexpr") << whitespace;
        if (ctor.is_consteval())
            output << keyword("consteval") << whitespace;

        output << identifier(ctor.semantic_scope()) << identifier(ctor.name());
        write_function_parameters(output, ctor);
        write_noexcept(output, ctor, output.formatting().is_set(formatting_flags::operator_ws));

        write_function_body(output, ctor, false);
    }
    return static_cast<bool>(output);
}

bool generate_destructor(code_generator& generator, const cpp_destructor& dtor,
                         cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(dtor), cur_access);
    if (output)
    {
        if (is_friended(dtor))
            output << keyword("friend") << whitespace;
        write_prefix_virtual(output, dtor.virtual_info());
        output << identifier(dtor.semantic_scope()) << identifier(dtor.name()) << punctuation("(")
               << punctuation(")");
        write_noexcept(output, dtor, output.formatting().is_set(formatting_flags::operator_ws));

        write_suffix_virtual(output, dtor.virtual_info(), dtor.is_definition());
        write_function_body(output, dtor, is_pure(dtor.virtual_info()));
    }
    return static_cast<bool>(output);
}

bool generate_function_base(code_generator& generator, const cpp_function_base& base,
                            cpp_access_specifier_kind          cur_access,
                            const cpp_template_specialization& spec)
{
    switch (base.kind())
    {
    case cpp_entity_kind::function_t:
        return generate_function(generator, static_cast<const cpp_function&>(base), cur_access,
                                 type_safe::ref(spec));
    case cpp_entity_kind::member_function_t:
        return generate_member_function(generator, static_cast<const cpp_member_function&>(base),
                                        cur_access, type_safe::ref(spec));
    case cpp_entity_kind::conversion_op_t:
        return generate_conversion_op(generator, static_cast<const cpp_conversion_op&>(base),
                                      cur_access);
    case cpp_entity_kind::constructor_t:
        return generate_constructor(generator, static_cast<const cpp_constructor&>(base),
                                    cur_access);

    default:
        DEBUG_UNREACHABLE(detail::assert_handler{});
        break;
    }
    return false;
}

bool generate_friend(code_generator& generator, const cpp_friend& f,
                     cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(f), cur_access);
    if (output)
    {
        if (auto e = f.entity())
            generate_code_impl(generator, e.value(), cur_access);
        else if (auto type = f.type())
        {
            output << keyword("friend") << whitespace;
            detail::write_type(output, type.value(), "");
            output << punctuation(";");
        }
        else
            DEBUG_UNREACHABLE(detail::assert_handler{});
    }
    return static_cast<bool>(output);
}

bool generate_template_type_parameter(code_generator&                    generator,
                                      const cpp_template_type_parameter& param,
                                      cpp_access_specifier_kind          cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(param), cur_access);
    if (output)
    {
        output << keyword(to_string(param.keyword()));
        if (param.is_variadic())
            output << operator_ws << punctuation("...");
        if (!param.name().empty())
            output << whitespace << identifier(param.name());
        if (param.default_type())
        {
            output << operator_ws << punctuation("=") << operator_ws;
            detail::write_type(output, param.default_type().value(), "");
        }
    }
    return static_cast<bool>(output);
}

bool generate_non_type_template_parameter(code_generator&                        generator,
                                          const cpp_non_type_template_parameter& param,
                                          cpp_access_specifier_kind              cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(param), cur_access);
    if (output)
    {
        detail::write_type(output, param.type(), param.name(), param.is_variadic());
        if (param.default_value())
        {
            output << operator_ws << punctuation("=") << operator_ws;
            detail::write_expression(output, param.default_value().value());
        }
    }
    return static_cast<bool>(output);
}

bool generate_template_template_parameter(code_generator&                        generator,
                                          const cpp_template_template_parameter& param,
                                          cpp_access_specifier_kind              cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(param), cur_access);
    if (output)
    {
        output << keyword("template") << operator_ws << punctuation("<") << bracket_ws;
        write_container(output, param.parameters(), punctuation(","), cur_access);
        output << bracket_ws << punctuation(">") << operator_ws
               << keyword(to_string(param.keyword()));
        if (param.is_variadic())
            output << operator_ws << punctuation("...");
        output << whitespace << identifier(param.name());
        if (param.default_template())
            output << operator_ws << punctuation("=") << operator_ws
                   << param.default_template().value();
    }
    return static_cast<bool>(output);
}

void write_template_parameters(code_generator::output& output, const cpp_template& templ,
                               bool hide_if_empty)
{
    if (!hide_if_empty)
        output << keyword("template") << operator_ws << punctuation("<") << bracket_ws;

    auto need_sep    = false;
    auto need_header = hide_if_empty;
    for (auto& param : templ.parameters())
    {
        auto is_excluded = output.options(param, cpp_public).is_set(code_generator::exclude);
        if (!is_excluded)
        {
            if (need_header)
            {
                need_header = false;
                output << keyword("template") << operator_ws << punctuation("<") << bracket_ws;
            }
            else if (need_sep)
                output << comma;
            need_sep = generate_code_impl(*output.generator(), param, cpp_public);
        }
    }

    if (!need_header)
        output << bracket_ws << punctuation(">") << newl;
}

bool generate_alias_template(code_generator& generator, const cpp_alias_template& alias,
                             cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(alias), cur_access);
    if (output)
    {
        write_template_parameters(output, alias, true);
        generate_code_impl(generator, alias.type_alias(), cur_access);
    }
    return static_cast<bool>(output);
}

bool generate_variable_template(code_generator& generator, const cpp_variable_template& var,
                                cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(var), cur_access);
    if (output)
    {
        write_template_parameters(output, var, true);
        generate_code_impl(generator, var.variable(), cur_access);
    }
    return static_cast<bool>(output);
}

bool generate_function_template(code_generator& generator, const cpp_function_template& func,
                                cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(func), cur_access);
    if (output)
    {
        write_template_parameters(output, func, true);
        generate_code_impl(generator, func.function(), cur_access);
    }
    return static_cast<bool>(output);
}

bool generate_function_template_specialization(code_generator& generator,
                                               const cpp_function_template_specialization& func,
                                               cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(func), cur_access);
    if (output)
    {
        DEBUG_ASSERT(func.is_full_specialization(), detail::assert_handler{});
        if (!is_friended(func))
            // don't write template parameters in friend
            write_template_parameters(output, func, false);
        generate_function_base(generator, func.function(), cur_access, func);
    }
    return static_cast<bool>(output);
}

bool generate_class_template(code_generator& generator, const cpp_class_template& templ,
                             cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(templ), cur_access);
    if (output)
    {
        write_template_parameters(output, templ, true);
        generate_class(generator, templ.class_(), cur_access);
    }
    return static_cast<bool>(output);
}

bool generate_class_template_specialization(code_generator&                          generator,
                                            const cpp_class_template_specialization& templ,
                                            cpp_access_specifier_kind                cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(templ), cur_access);
    if (output)
    {
        write_template_parameters(output, templ, false);
        generate_class(generator, templ.class_(), cur_access, type_safe::ref(templ));
    }
    return static_cast<bool>(output);
}

bool generate_static_assert(code_generator& generator, const cpp_static_assert& assert,
                            cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(assert), cur_access);
    if (output)
    {
        output << keyword("static_assert") << punctuation("(") << bracket_ws;
        detail::write_expression(output, assert.expression());
        output << comma << string_literal('"' + assert.message() + '"');
        output << bracket_ws << punctuation(");") << newl;
    }
    return static_cast<bool>(output);
}

bool generate_unexposed(code_generator& generator, const cpp_unexposed_entity& entity,
                        cpp_access_specifier_kind cur_access)
{
    code_generator::output output(type_safe::ref(generator), type_safe::ref(entity), cur_access);
    if (output)
    {
        detail::write_token_string(output, entity.spelling());
        output << newl;
    }
    return static_cast<bool>(output);
}

bool generate_code_impl(code_generator& generator, const cpp_entity& e,
                        cpp_access_specifier_kind cur_access)
{
    switch (e.kind())
    {
#define CPPAST_DETAIL_HANDLE(Name)                                                                 \
    case cpp_entity_kind::Name##_t:                                                                \
        return generate_##Name(generator, static_cast<const cpp_##Name&>(e), cur_access);

        CPPAST_DETAIL_HANDLE(file)

        CPPAST_DETAIL_HANDLE(macro_parameter)
        CPPAST_DETAIL_HANDLE(macro_definition)
        CPPAST_DETAIL_HANDLE(include_directive)

        CPPAST_DETAIL_HANDLE(language_linkage)
        CPPAST_DETAIL_HANDLE(namespace)
        CPPAST_DETAIL_HANDLE(namespace_alias)
        CPPAST_DETAIL_HANDLE(using_directive)
        CPPAST_DETAIL_HANDLE(using_declaration)

        CPPAST_DETAIL_HANDLE(type_alias)

        CPPAST_DETAIL_HANDLE(enum)
        CPPAST_DETAIL_HANDLE(enum_value)

        CPPAST_DETAIL_HANDLE(class)
        CPPAST_DETAIL_HANDLE(access_specifier)
        CPPAST_DETAIL_HANDLE(base_class)

        CPPAST_DETAIL_HANDLE(variable)
        CPPAST_DETAIL_HANDLE(member_variable)
        CPPAST_DETAIL_HANDLE(bitfield)

        CPPAST_DETAIL_HANDLE(function_parameter)
        CPPAST_DETAIL_HANDLE(function)
        CPPAST_DETAIL_HANDLE(member_function)
        CPPAST_DETAIL_HANDLE(conversion_op)
        CPPAST_DETAIL_HANDLE(constructor)
        CPPAST_DETAIL_HANDLE(destructor)

        CPPAST_DETAIL_HANDLE(friend)

        CPPAST_DETAIL_HANDLE(template_type_parameter)
        CPPAST_DETAIL_HANDLE(non_type_template_parameter)
        CPPAST_DETAIL_HANDLE(template_template_parameter)

        CPPAST_DETAIL_HANDLE(alias_template)
        CPPAST_DETAIL_HANDLE(variable_template)
        CPPAST_DETAIL_HANDLE(function_template)
        CPPAST_DETAIL_HANDLE(function_template_specialization)
        CPPAST_DETAIL_HANDLE(class_template)
        CPPAST_DETAIL_HANDLE(class_template_specialization)

        CPPAST_DETAIL_HANDLE(static_assert)

    case cpp_entity_kind::unexposed_t:
        return generate_unexposed(generator, static_cast<const cpp_unexposed_entity&>(e),
                                  cur_access);

#undef CPPAST_DETAIL_HANDLE

    case cpp_entity_kind::count:
        DEBUG_UNREACHABLE(detail::assert_handler{});
        break;
    }

    return false;
}
} // namespace

bool code_generator::generate_code(const cpp_entity& entity)
{
    return generate_code_impl(*this, entity, cpp_public);
}

bool cppast::generate_code(code_generator& generator, const cpp_entity& e)
{
    generator.main_entity_ = type_safe::ref(e);
    auto result            = generate_code_impl(generator, e, cpp_public);
    generator.main_entity_ = nullptr;
    return result;
}

void detail::write_template_arguments(
    code_generator::output&                                                output,
    type_safe::optional<type_safe::array_ref<const cpp_template_argument>> arguments)
{
    if (!arguments)
    {
        output << punctuation("<") << punctuation(">");
    }
    else
    {
        output << punctuation("<") << bracket_ws;
        auto need_sep = false;
        for (auto& arg : arguments.value())
        {
            if (need_sep)
                output << comma;
            else
                need_sep = true;

            if (auto type = arg.type())
                detail::write_type(output, type.value(), "");
            else if (auto expr = arg.expression())
                detail::write_expression(output, expr.value());
            else if (auto templ = arg.template_ref())
                output << templ.value();
            else
                DEBUG_UNREACHABLE(detail::assert_handler{});
        }
        output << bracket_ws << punctuation(">");
    }
}

void detail::write_token_string(code_generator::output& output, const cpp_token_string& tokens)
{
    auto last_kind = cpp_token_kind::punctuation; // neutral regarding whitespace
    for (auto& token : tokens)
    {
        switch (token.kind)
        {
        case cpp_token_kind::identifier:
            if (last_kind == cpp_token_kind::identifier || last_kind == cpp_token_kind::keyword)
                output << whitespace;
            output << keyword(token.spelling);
            break;

        case cpp_token_kind::keyword:
            if (last_kind == cpp_token_kind::identifier || last_kind == cpp_token_kind::keyword)
                output << whitespace;
            output << identifier(token.spelling);
            if (token.spelling == "template")
                output << operator_ws;
            break;

        case cpp_token_kind::int_literal:
            output << int_literal(token.spelling);
            break;
        case cpp_token_kind::float_literal:
            output << float_literal(token.spelling);
            break;
        case cpp_token_kind::char_literal:
        case cpp_token_kind::string_literal:
            output << string_literal(token.spelling);
            break;

        case cpp_token_kind::punctuation:
            if (token.spelling == ",")
                output << comma;
            // print brackets
            // don't treat <> special as they might be operators...
            else if (token.spelling == "(" || token.spelling == "[" || token.spelling == "{")
                output << punctuation(token.spelling) << bracket_ws;
            else if (token.spelling == ")" || token.spelling == "]" || token.spelling == "}")
                output << bracket_ws << punctuation(token.spelling);
            // operators that are always binary operators in all contexts
            else if (token.spelling.back() == '=' // all assignment operators
                     || token.spelling == "/" || token.spelling == "%" || token.spelling == "=="
                     || token.spelling == "!=" || token.spelling == "<=" || token.spelling == ">="
                     || token.spelling == "&&" || token.spelling == "||" || token.spelling == "|"
                     || token.spelling == "^" || token.spelling == "?")
                output << operator_ws << punctuation(token.spelling) << operator_ws;
            else
                output << punctuation(token.spelling);
            break;
        }

        last_kind = token.kind;
    }
}
