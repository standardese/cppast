// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_template.hpp>

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

bool detail::cpp_type_ref_predicate::operator()(const cpp_entity& e)
{
    return is_type(e.kind());
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
