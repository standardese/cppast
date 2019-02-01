// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_decltype_type.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_type_alias.hpp>

using namespace cppast;

const char* cppast::to_string(cpp_builtin_type_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_void:
        return "void";

    case cpp_bool:
        return "bool";

    case cpp_uchar:
        return "unsigned char";
    case cpp_ushort:
        return "unsigned short";
    case cpp_uint:
        return "unsigned int";
    case cpp_ulong:
        return "unsigned long";
    case cpp_ulonglong:
        return "unsigned long long";
    case cpp_uint128:
        return "unsigned __int128";

    case cpp_schar:
        return "signed char";
    case cpp_short:
        return "short";
    case cpp_int:
        return "int";
    case cpp_long:
        return "long";
    case cpp_longlong:
        return "long long";
    case cpp_int128:
        return "__int128";

    case cpp_float:
        return "float";
    case cpp_double:
        return "double";
    case cpp_longdouble:
        return "long double";
    case cpp_float128:
        return "__float128";

    case cpp_char:
        return "char";
    case cpp_wchar:
        return "wchar_t";
    case cpp_char16:
        return "char16_t";
    case cpp_char32:
        return "char32_t";

    case cpp_nullptr:
        return "std::nullptr_t";
    }
    DEBUG_UNREACHABLE(detail::assert_handler{});
    return "__ups";
}

const cpp_type& cppast::remove_cv(const cpp_type& type) noexcept
{
    if (type.kind() == cpp_type_kind::cv_qualified_t)
    {
        auto& cv = static_cast<const cpp_cv_qualified_type&>(type);
        return cv.type();
    }

    return type;
}

const cpp_type& cppast::remove_const(const cpp_type& type) noexcept
{
    if (type.kind() == cpp_type_kind::cv_qualified_t)
    {
        auto& cv = static_cast<const cpp_cv_qualified_type&>(type);
        if (is_const(cv.cv_qualifier()))
            return cv.type();
    }
    return type;
}

const cpp_type& cppast::remove_volatile(const cpp_type& type) noexcept
{
    if (type.kind() == cpp_type_kind::cv_qualified_t)
    {
        auto& cv = static_cast<const cpp_cv_qualified_type&>(type);
        if (is_volatile(cv.cv_qualifier()))
            return cv.type();
    }
    return type;
}

bool detail::cpp_type_ref_predicate::operator()(const cpp_entity& e)
{
    switch (e.kind())
    {
    case cpp_entity_kind::type_alias_t:
    case cpp_entity_kind::enum_t:
    case cpp_entity_kind::class_t:
        return true;

    case cpp_entity_kind::file_t:
    case cpp_entity_kind::macro_parameter_t:
    case cpp_entity_kind::macro_definition_t:
    case cpp_entity_kind::include_directive_t:
    case cpp_entity_kind::language_linkage_t:
    case cpp_entity_kind::namespace_t:
    case cpp_entity_kind::namespace_alias_t:
    case cpp_entity_kind::using_directive_t:
    case cpp_entity_kind::using_declaration_t:
    case cpp_entity_kind::enum_value_t:
    case cpp_entity_kind::access_specifier_t:
    case cpp_entity_kind::base_class_t:
    case cpp_entity_kind::variable_t:
    case cpp_entity_kind::member_variable_t:
    case cpp_entity_kind::bitfield_t:
    case cpp_entity_kind::function_parameter_t:
    case cpp_entity_kind::function_t:
    case cpp_entity_kind::member_function_t:
    case cpp_entity_kind::conversion_op_t:
    case cpp_entity_kind::constructor_t:
    case cpp_entity_kind::destructor_t:
    case cpp_entity_kind::friend_t:
    case cpp_entity_kind::template_type_parameter_t:
    case cpp_entity_kind::non_type_template_parameter_t:
    case cpp_entity_kind::template_template_parameter_t:
    case cpp_entity_kind::alias_template_t:
    case cpp_entity_kind::variable_template_t:
    case cpp_entity_kind::function_template_t:
    case cpp_entity_kind::function_template_specialization_t:
    case cpp_entity_kind::class_template_t:
    case cpp_entity_kind::class_template_specialization_t:
    case cpp_entity_kind::static_assert_t:
    case cpp_entity_kind::unexposed_t:
    case cpp_entity_kind::count:
        break;
    }

    return false;
}

std::unique_ptr<cpp_dependent_type> cpp_dependent_type::build(
    std::string name, std::unique_ptr<cpp_template_parameter_type> dependee)
{
    return std::unique_ptr<cpp_dependent_type>(
        new cpp_dependent_type(std::move(name), std::move(dependee)));
}

std::unique_ptr<cpp_dependent_type> cpp_dependent_type::build(
    std::string name, std::unique_ptr<cpp_template_instantiation_type> dependee)
{
    return std::unique_ptr<cpp_dependent_type>(
        new cpp_dependent_type(std::move(name), std::move(dependee)));
}

namespace
{
// is directly a complex type
// is_complex_type also checks for children
bool is_direct_complex(const cpp_type& type) noexcept
{
    switch (type.kind())
    {
    case cpp_type_kind::builtin_t:
    case cpp_type_kind::user_defined_t:
    case cpp_type_kind::auto_t:
    case cpp_type_kind::decltype_t:
    case cpp_type_kind::decltype_auto_t:
    case cpp_type_kind::cv_qualified_t:
    case cpp_type_kind::pointer_t:
    case cpp_type_kind::reference_t:
    case cpp_type_kind::template_parameter_t:
    case cpp_type_kind::template_instantiation_t:
    case cpp_type_kind::dependent_t:
    case cpp_type_kind::unexposed_t:
        return false;

    case cpp_type_kind::array_t:
    case cpp_type_kind::function_t:
    case cpp_type_kind::member_function_t:
    case cpp_type_kind::member_object_t:
        return true;
    }

    DEBUG_UNREACHABLE(detail::assert_handler{});
    return false;
}
} // namespace

bool detail::is_complex_type(const cpp_type& type) noexcept
{
    switch (type.kind())
    {
    case cpp_type_kind::cv_qualified_t:
        return is_complex_type(static_cast<const cpp_cv_qualified_type&>(type).type());
    case cpp_type_kind::pointer_t:
        return is_complex_type(static_cast<const cpp_pointer_type&>(type).pointee());
    case cpp_type_kind::reference_t:
        return is_complex_type(static_cast<const cpp_reference_type&>(type).referee());

    default:
        break;
    }

    return is_direct_complex(type);
}

namespace
{
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

void write_builtin(code_generator::output& output, const cpp_builtin_type& type)
{
    output << keyword(to_string(type.builtin_type_kind()));
}

void write_user_defined(code_generator::output& output, const cpp_user_defined_type& type)
{
    output << type.entity();
}

void write_auto(code_generator::output& output, const cpp_auto_type&)
{
    output << keyword("auto");
}

void write_decltype(code_generator::output& output, const cpp_decltype_type& type)
{
    output << keyword("decltype") << punctuation("(") << bracket_ws;
    detail::write_expression(output, type.expression());
    output << bracket_ws << punctuation(")");
}

void write_decltype_auto(code_generator::output& output, const cpp_decltype_auto_type&)
{
    output << keyword("decltype") << punctuation("(") << bracket_ws << keyword("auto") << bracket_ws
           << punctuation(")");
}

void write_cv_qualified_prefix(code_generator::output& output, const cpp_cv_qualified_type& type)
{
    detail::write_type_prefix(output, type.type());

    if (is_direct_complex(type.type()))
        output << punctuation("(") << bracket_ws;

    if (is_const(type.cv_qualifier()))
        output << whitespace << keyword("const");
    if (is_volatile(type.cv_qualifier()))
        output << whitespace << keyword("volatile");
}

void write_cv_qualified_suffix(code_generator::output& output, const cpp_cv_qualified_type& type)
{
    if (is_direct_complex(type.type()))
        output << bracket_ws << punctuation(")");
    detail::write_type_suffix(output, type.type());
}

bool pointer_requires_paren(const cpp_pointer_type& type)
{
    auto kind = type.pointee().kind();
    return kind == cpp_type_kind::function_t || kind == cpp_type_kind::array_t;
}

void write_pointer_prefix(code_generator::output& output, const cpp_pointer_type& type)
{
    detail::write_type_prefix(output, type.pointee());

    if (pointer_requires_paren(type))
        output << punctuation("(") << bracket_ws;
    else if (output.formatting().is_set(formatting_flags::ptr_ref_var))
        output << whitespace;

    output << punctuation("*");
}

void write_pointer_suffix(code_generator::output& output, const cpp_pointer_type& type)
{
    if (pointer_requires_paren(type))
        output << bracket_ws << punctuation(")");
    detail::write_type_suffix(output, type.pointee());
}

void write_reference_prefix(code_generator::output& output, const cpp_reference_type& type)
{
    detail::write_type_prefix(output, type.referee());

    if (is_direct_complex(type.referee()))
        output << punctuation("(") << bracket_ws;
    else if (output.formatting().is_set(formatting_flags::ptr_ref_var))
        output << whitespace;

    if (type.reference_kind() == cpp_ref_lvalue)
        output << punctuation("&");
    else if (type.reference_kind() == cpp_ref_rvalue)
        output << punctuation("&&");
    else
        DEBUG_UNREACHABLE(detail::assert_handler{});
}

void write_reference_suffix(code_generator::output& output, const cpp_reference_type& type)
{
    if (is_direct_complex(type.referee()))
        output << bracket_ws << punctuation(")");
    detail::write_type_suffix(output, type.referee());
}

void write_array_prefix(code_generator::output& output, const cpp_array_type& type)
{
    detail::write_type_prefix(output, type.value_type());
}

void write_array_suffix(code_generator::output& output, const cpp_array_type& type)
{
    output << punctuation("[");
    if (type.size())
    {
        output << bracket_ws;
        detail::write_expression(output, type.size().value());
        output << bracket_ws;
    }
    output << punctuation("]");
    detail::write_type_suffix(output, type.value_type());
}

void write_function_prefix(code_generator::output& output, const cpp_function_type& type)
{
    detail::write_type_prefix(output, type.return_type());
}

template <typename T>
void write_parameters(code_generator::output& output, const T& type)
{
    output << punctuation("(") << bracket_ws;

    auto need_sep = false;
    for (auto& param : type.parameter_types())
    {
        if (need_sep)
            output << comma;
        else
            need_sep = true;
        detail::write_type_prefix(output, param);
        detail::write_type_suffix(output, param);
    }
    if (type.is_variadic())
    {
        if (need_sep)
            output << comma;
        output << punctuation("...");
    }

    output << bracket_ws << punctuation(")");
}

void write_function_suffix(code_generator::output& output, const cpp_function_type& type)
{
    write_parameters(output, type);

    detail::write_type_suffix(output, type.return_type());
}

const cpp_type& strip_class_type(const cpp_type& type, cpp_cv* cv, cpp_reference* ref)
{
    if (type.kind() == cpp_type_kind::cv_qualified_t)
    {
        auto& cv_qual = static_cast<const cpp_cv_qualified_type&>(type);
        if (cv)
            *cv = cv_qual.cv_qualifier();
        return strip_class_type(cv_qual.type(), cv, ref);
    }
    else if (type.kind() == cpp_type_kind::reference_t)
    {
        auto& ref_type = static_cast<const cpp_reference_type&>(type);
        if (ref)
            *ref = ref_type.reference_kind();
        return strip_class_type(ref_type.referee(), cv, ref);
    }
    else
    {
        DEBUG_ASSERT(!detail::is_complex_type(type), detail::assert_handler{});
        return type;
    }
}

void write_member_function_prefix(code_generator::output&         output,
                                  const cpp_member_function_type& type)
{
    detail::write_type_prefix(output, type.return_type());

    output << punctuation("(") << bracket_ws;
    detail::write_type_prefix(output, strip_class_type(type.class_type(), nullptr, nullptr));
    output << punctuation("::");
}

void write_member_function_suffix(code_generator::output&         output,
                                  const cpp_member_function_type& type)
{
    output << bracket_ws << punctuation(")");
    write_parameters(output, type);

    auto cv  = cpp_cv_none;
    auto ref = cpp_ref_none;
    strip_class_type(type.class_type(), &cv, &ref);

    if (cv == cpp_cv_const_volatile)
        output << keyword("const") << whitespace << keyword("volatile");
    else if (is_const(cv))
        output << keyword("const");
    else if (is_volatile(cv))
        output << keyword("volatile");

    if (ref == cpp_ref_lvalue)
        output << operator_ws << punctuation("&") << operator_ws;
    else if (ref == cpp_ref_rvalue)
        output << operator_ws << punctuation("&&") << operator_ws;

    detail::write_type_suffix(output, type.return_type());
}

void write_member_object_prefix(code_generator::output& output, const cpp_member_object_type& type)
{
    detail::write_type_prefix(output, type.object_type());
    output << punctuation("(") << bracket_ws;
    DEBUG_ASSERT(!detail::is_complex_type(type.class_type()), detail::assert_handler{});
    detail::write_type_prefix(output, type.class_type());
    output << punctuation("::");
}

void write_member_object_suffix(code_generator::output& output, const cpp_member_object_type&)
{
    output << bracket_ws << punctuation(")");
}

void write_template_parameter(code_generator::output&            output,
                              const cpp_template_parameter_type& type)
{
    output << type.entity();
}

void write_template_instantiation(code_generator::output&                output,
                                  const cpp_template_instantiation_type& type)
{
    output << type.primary_template();
    if (output.was_reference_excluded())
        return;

    if (type.arguments_exposed())
        detail::write_template_arguments(output, type.arguments());
    else
        output << punctuation("<") << bracket_ws << token_seq(type.unexposed_arguments())
               << bracket_ws << punctuation(">");
}

void write_dependent(code_generator::output& output, const cpp_dependent_type& type)
{
    output << token_seq(type.name());
}

void write_unexposed(code_generator::output& output, const cpp_unexposed_type& type)
{
    output << token_seq(type.name());
}
} // namespace

void detail::write_type_prefix(code_generator::output& output, const cpp_type& type)
{
    switch (type.kind())
    {
#define CPPAST_DETAIL_HANDLE(Name)                                                                 \
    case cpp_type_kind::Name##_t:                                                                  \
        write_##Name(output, static_cast<const cpp_##Name##_type&>(type));                         \
        break;

#define CPPAST_DETAIL_HANDLE_COMPLEX(Name)                                                         \
    case cpp_type_kind::Name##_t:                                                                  \
        write_##Name##_prefix(output, static_cast<const cpp_##Name##_type&>(type));                \
        break;

        CPPAST_DETAIL_HANDLE(builtin)
        CPPAST_DETAIL_HANDLE(user_defined)
        CPPAST_DETAIL_HANDLE(auto)
        CPPAST_DETAIL_HANDLE(decltype)
        CPPAST_DETAIL_HANDLE(decltype_auto)
        CPPAST_DETAIL_HANDLE_COMPLEX(cv_qualified)
        CPPAST_DETAIL_HANDLE_COMPLEX(pointer)
        CPPAST_DETAIL_HANDLE_COMPLEX(reference)
        CPPAST_DETAIL_HANDLE_COMPLEX(array)
        CPPAST_DETAIL_HANDLE_COMPLEX(function)
        CPPAST_DETAIL_HANDLE_COMPLEX(member_function)
        CPPAST_DETAIL_HANDLE_COMPLEX(member_object)
        CPPAST_DETAIL_HANDLE(template_parameter)
        CPPAST_DETAIL_HANDLE(template_instantiation)
        CPPAST_DETAIL_HANDLE(dependent)
        CPPAST_DETAIL_HANDLE(unexposed)
    }

#undef CPPAST_DETAIL_HANDLE
#undef CPPAST_DETAIL_HANDLE_COMPLEX
}

void detail::write_type_suffix(code_generator::output& output, const cpp_type& type)
{
    switch (type.kind())
    {
#define CPPAST_DETAIL_HANDLE(Name)                                                                 \
    case cpp_type_kind::Name##_t:                                                                  \
        break;

#define CPPAST_DETAIL_HANDLE_COMPLEX(Name)                                                         \
    case cpp_type_kind::Name##_t:                                                                  \
        write_##Name##_suffix(output, static_cast<const cpp_##Name##_type&>(type));                \
        break;

        CPPAST_DETAIL_HANDLE(builtin)
        CPPAST_DETAIL_HANDLE(user_defined)
        CPPAST_DETAIL_HANDLE(auto)
        CPPAST_DETAIL_HANDLE(decltype)
        CPPAST_DETAIL_HANDLE(decltype_auto)
        CPPAST_DETAIL_HANDLE_COMPLEX(cv_qualified)
        CPPAST_DETAIL_HANDLE_COMPLEX(pointer)
        CPPAST_DETAIL_HANDLE_COMPLEX(reference)
        CPPAST_DETAIL_HANDLE_COMPLEX(array)
        CPPAST_DETAIL_HANDLE_COMPLEX(function)
        CPPAST_DETAIL_HANDLE_COMPLEX(member_function)
        CPPAST_DETAIL_HANDLE_COMPLEX(member_object)
        CPPAST_DETAIL_HANDLE(template_parameter)
        CPPAST_DETAIL_HANDLE(template_instantiation)
        CPPAST_DETAIL_HANDLE(dependent)
        CPPAST_DETAIL_HANDLE(unexposed)
    }

#undef CPPAST_DETAIL_HANDLE
#undef CPPAST_DETAIL_HANDLE_COMPLEX
}

void detail::write_type(code_generator::output& output, const cpp_type& type, std::string name,
                        bool is_variadic)
{
    write_type_prefix(output, type);
    if (!name.empty())
        output << whitespace << identifier(name);
    if (is_variadic)
        output << operator_ws << punctuation("...") << operator_ws;
    write_type_suffix(output, type);
}

std::string cppast::to_string(const cpp_type& type)
{
    class to_string_generator : public code_generator
    {
    public:
        std::string get()
        {
            return std::move(result_);
        }

    private:
        void do_indent() override {}

        void do_unindent() override {}

        void do_write_token_seq(string_view tokens) override
        {
            result_ += tokens.c_str();
        }

        std::string result_;
    } generator;

    // just a dummy type for the output
    static auto dummy_entity = cpp_type_alias::build("foo", cpp_builtin_type::build(cpp_int));
    to_string_generator::output output(type_safe::ref(generator), type_safe::ref(*dummy_entity),
                                       cpp_public);
    write_type(output, type, "");
    return generator.get();
}
