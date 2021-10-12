// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_decltype_type.hpp>
#include <cppast/cpp_expression.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_template_parameter.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_type_alias.hpp>

using namespace cppast;

namespace
{
type_safe::optional<cpp_builtin_type_kind> get_builtin_type_kind(std::string_view qual_type)
{
    if (qual_type == "void")
        return cpp_void;
    else if (qual_type == "bool")
        return cpp_bool;
    else if (qual_type == "unsigned char")
        return cpp_uchar;
    else if (qual_type == "unsigned short")
        return cpp_ushort;
    else if (qual_type == "unsigned int")
        return cpp_uint;
    else if (qual_type == "unsigned long")
        return cpp_ulong;
    else if (qual_type == "unsigned long long")
        return cpp_ulonglong;
    else if (qual_type == "unsigned __int128")
        return cpp_uint128;
    else if (qual_type == "signed char")
        return cpp_schar;
    else if (qual_type == "short")
        return cpp_short;
    else if (qual_type == "int")
        return cpp_int;
    else if (qual_type == "long")
        return cpp_long;
    else if (qual_type == "long long")
        return cpp_longlong;
    else if (qual_type == "__int128")
        return cpp_int128;
    else if (qual_type == "float")
        return cpp_float;
    else if (qual_type == "double")
        return cpp_double;
    else if (qual_type == "long double")
        return cpp_longdouble;
    else if (qual_type == "__float128")
        return cpp_float128;
    else if (qual_type == "char")
        return cpp_char;
    else if (qual_type == "wchar_t")
        return cpp_wchar;
    else if (qual_type == "char16_t")
        return cpp_char16;
    else if (qual_type == "char32_t")
        return cpp_char32;
    else
        return type_safe::nullopt;
}

cpp_cv get_cv(std::string_view qualifiers)
{
    auto is_const    = qualifiers.find("const") != std::string_view::npos;
    auto is_volatile = qualifiers.find("volatile") != std::string_view::npos;

    if (is_const && is_volatile)
        return cpp_cv_const_volatile;
    else if (is_const)
        return cpp_cv_const;
    else if (is_volatile)
        return cpp_cv_volatile;
    else
        return cpp_cv_none;
}

template <typename FunctionBuilder, typename... Args>
auto build_function_type(astdump_detail::parse_context& context,
                         simdjson::ondemand::value      proto_type, Args&&... args)
{
    auto is_variadic = [&] {
        auto field = proto_type["variadic"].get_bool();
        if (field.error() != simdjson::NO_SUCH_FIELD)
            return field.value();
        else
            return false;
    }();

    auto inner_types = proto_type["inner"];
    auto inner_begin = inner_types.begin();
    auto inner_end   = inner_types.end();
    auto return_type = parse_type(context, *inner_begin);

    auto builder = FunctionBuilder(std::forward<Args>(args)..., std::move(return_type));
    if (is_variadic)
        builder.is_variadic();

    for (auto iter = ++inner_begin; iter != inner_end; ++iter)
        builder.add_parameter(parse_type(context, *iter));

    return builder.finish();
}
} // namespace

std::unique_ptr<cpp_type> astdump_detail::parse_type(parse_context& context, dom::value type)
{
    auto kind      = type["kind"].get_string().value();
    auto qual_type = type["type"]["qualType"].get_string().value();

    if (kind == "BuiltinType")
    {
        auto builtin_type = get_builtin_type_kind(qual_type);
        if (builtin_type.has_value())
            return cpp_builtin_type::build(builtin_type.value());
        else
            return cpp_unexposed_type::build(std::string(qual_type));
    }
    else if (kind == "RecordType" || kind == "EnumType" || kind == "TypedefType")
    {
        auto target      = type["decl"].get_object().value();
        auto target_id   = get_entity_id(context, target);
        auto target_name = target["name"].get_string().value();

        return cpp_user_defined_type::build(cpp_type_ref(target_id, std::string(target_name)));
    }
    else if (kind == "ElaboratedType") // std::foo or struct bar
    {
        // The namespace prefix, e.g. `std::`.
        auto qualifier = [&] {
            auto field = type["qualifier"].get_string();
            if (field.error() != simdjson::NO_SUCH_FIELD)
                return field.value();
            else
                return std::string_view();
        }();

        auto inner = parse_type(context, type["inner"].at(0));
        DEBUG_ASSERT(inner->kind() == cpp_type_kind::user_defined_t, detail::assert_handler{});
        if (qualifier.empty())
            return inner;
        auto& udt = static_cast<cpp_user_defined_type&>(*inner);

        // Keep the target, but add the namespace.
        auto target_id = *udt.entity().id().begin();
        return cpp_user_defined_type::build(
            cpp_type_ref(target_id, std::string(qualifier) + udt.entity().name()));
    }
    else if (kind == "DecltypeType")
    {
        auto expr = parse_expression(context, type["inner"].at(0));
        return cpp_decltype_type::build(std::move(expr));
    }
    else if (kind == "QualType")
    {
        auto qualifiers = type["qualifiers"].get_string().value();
        auto inner      = parse_type(context, type["inner"].at(0));
        return cpp_cv_qualified_type::build(std::move(inner), get_cv(qualifiers));
    }
    else if (kind == "PointerType")
    {
        auto inner = parse_type(context, type["inner"].at(0));
        return cpp_pointer_type::build(std::move(inner));
    }
    else if (kind == "LValueReferenceType")
    {
        auto inner = parse_type(context, type["inner"].at(0));
        return cpp_reference_type::build(std::move(inner), cpp_ref_lvalue);
    }
    else if (kind == "RValueReferenceType")
    {
        auto inner = parse_type(context, type["inner"].at(0));
        return cpp_reference_type::build(std::move(inner), cpp_ref_rvalue);
    }
    else if (kind == "ConstantArrayType")
    {
        auto size  = type["size"].get_uint64().value();
        auto inner = parse_type(context, type["inner"].at(0));

        auto size_type = cpp_builtin_type::build(cpp_int);
        auto size_expr = cpp_literal_expression::build(std::move(size_type), std::to_string(size));
        return cpp_array_type::build(std::move(inner), std::move(size_expr));
    }
    else if (kind == "IncompleteArrayType")
    {
        auto inner = parse_type(context, type["inner"].at(0));
        return cpp_array_type::build(std::move(inner), nullptr);
    }
    else if (kind == "ParenType")
    {
        // It has the actual FFunctionProtoType we care about as first inner type.
        return parse_type(context, type["inner"].at(0));
    }
    else if (kind == "FunctionProtoType")
    {
        return build_function_type<cpp_function_type::builder>(context, type);
    }
    else if (kind == "MemberPointerType")
    {
        if ([&] {
                auto field = type["isFunction"].get_bool();
                return field.error() != simdjson::NO_SUCH_FIELD && field.value();
            }())
        {
            auto inner       = type["inner"];
            auto inner_begin = inner.begin();
            auto inner_end   = inner.end();

            auto class_type = parse_type(context, *inner_begin);
            ++inner_begin;
            auto proto_type = (*inner_begin)["inner"].at(0).value();

            auto cv = [&] {
                auto const_   = proto_type["const"].get_bool();
                auto is_const = const_.error() != simdjson::NO_SUCH_FIELD && const_.value();

                auto volatile_ = proto_type["volatile"].get_bool();
                auto is_volatile
                    = volatile_.error() != simdjson::NO_SUCH_FIELD && volatile_.value();

                if (is_const && is_volatile)
                    return cpp_cv_const_volatile;
                else if (is_const)
                    return cpp_cv_const;
                else if (is_volatile)
                    return cpp_cv_volatile;
                else
                    return cpp_cv_none;
            }();
            if (cv != cpp_cv_none)
                class_type = cpp_cv_qualified_type::build(std::move(class_type), cv);

            auto ref = [&] {
                auto field = proto_type["refQualifier"].get_string();
                if (field.error() == simdjson::NO_SUCH_FIELD)
                    return cpp_ref_none;
                else if (field.value() == "&")
                    return cpp_ref_lvalue;
                else if (field.value() == "&&")
                    return cpp_ref_rvalue;
                else
                    return cpp_ref_none;
            }();
            if (ref != cpp_ref_none)
                class_type = cpp_reference_type::build(std::move(class_type), ref);

            auto result
                = build_function_type<cpp_member_function_type::builder>(context, proto_type,
                                                                         std::move(class_type));
            return cpp_pointer_type::build(std::move(result));
        }
        else if ([&] {
                     auto field = type["isData"].get_bool();
                     return field.error() != simdjson::NO_SUCH_FIELD && field.value();
                 }())
        {
            auto inner       = type["inner"];
            auto inner_begin = inner.begin();
            auto inner_end   = inner.end();

            auto class_type = parse_type(context, *inner_begin);
            ++inner_begin;
            auto object_type = parse_type(context, (*inner_begin)["inner"].at(0).value());

            auto result
                = cpp_member_object_type::build(std::move(class_type), std::move(object_type));
            return cpp_pointer_type::build(std::move(result));
        }
        else
        {
            return cpp_unexposed_type::build(std::string(qual_type));
        }
    }

    return cpp_unexposed_type::build(std::string(qual_type));
}

std::unique_ptr<cpp_entity> astdump_detail::parse_type_alias(parse_context& context,
                                                             dom::object    entity)
{
    auto id   = get_entity_id(context, entity);
    auto name = entity["name"].get_string().value();
    auto type = parse_type(context, entity["inner"].at(0));

    auto result = cpp_type_alias::build(*context.idx, id, std::string(name), std::move(type));
    handle_comment_child(context, *result, entity);
    return result;
}

