// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/cpp_type_alias.hpp>

using namespace cppast;

namespace
{
    void remove_trailing_ws(std::string& str)
    {
        while (!str.empty() && std::isspace(str.back()))
            str.pop_back();
    }

    void remove_leading_ws(std::string& str)
    {
        while (!str.empty() && std::isspace(str.front()))
            str.erase(0, 1);
    }

    bool remove_suffix(std::string& str, const char* suffix)
    {
        auto length = std::strlen(suffix);
        if (str.length() >= length && str.compare(str.length() - length, length, suffix) == 0)
        {
            str.erase(str.length() - length);
            remove_trailing_ws(str);
            return true;
        }
        else
            return false;
    }

    bool remove_prefix(std::string& str, const char* prefix)
    {
        auto length = std::strlen(prefix);
        if (str.length() >= length && str.compare(0u, length, prefix) == 0)
        {
            str.erase(0u, length);
            remove_leading_ws(str);
            return true;
        }
        else
            return false;
    }

    std::string get_type_spelling(const CXType& type)
    {
        return detail::cxstring(clang_getTypeSpelling(type)).c_str();
    }

    // const/volatile at the end (because people do that apparently!)
    cpp_cv suffix_cv(std::string& spelling)
    {
        auto cv = cpp_cv_none;
        if (remove_suffix(spelling, "const"))
        {
            if (remove_suffix(spelling, "volatile"))
                cv = cpp_cv_const_volatile;
            else
                cv = cpp_cv_const;
        }
        else if (remove_suffix(spelling, "volatile"))
        {
            if (remove_suffix(spelling, "const"))
                cv = cpp_cv_const_volatile;
            else
                cv = cpp_cv_volatile;
        }

        return cv;
    }

    // const/volatile at the beginning
    // (weird that the better version is less performant, isn't it?)
    cpp_cv prefix_cv(std::string& spelling)
    {
        auto cv = cpp_cv_none;
        if (remove_prefix(spelling, "const"))
        {
            if (remove_prefix(spelling, "volatile"))
                cv = cpp_cv_const_volatile;
            else
                cv = cpp_cv_const;
        }
        else if (remove_prefix(spelling, "volatile"))
        {
            if (remove_prefix(spelling, "const"))
                cv = cpp_cv_const_volatile;
            else
                cv = cpp_cv_volatile;
        }

        return cv;
    }

    cpp_cv merge_cv(cpp_cv a, cpp_cv b)
    {
        switch (a)
        {
        case cpp_cv_none:
            return b;

        case cpp_cv_const:
            switch (b)
            {
            case cpp_cv_none:
                return cpp_cv_const;
            case cpp_cv_const:
                return cpp_cv_const;
            case cpp_cv_volatile:
                return cpp_cv_const_volatile;
            case cpp_cv_const_volatile:
                return cpp_cv_const_volatile;
            }
            break;

        case cpp_cv_volatile:
            switch (b)
            {
            case cpp_cv_none:
                return cpp_cv_volatile;
            case cpp_cv_const:
                return cpp_cv_const_volatile;
            case cpp_cv_volatile:
                return cpp_cv_volatile;
            case cpp_cv_const_volatile:
                return cpp_cv_const_volatile;
            }
            break;

        case cpp_cv_const_volatile:
            return cpp_cv_const_volatile;
        }

        DEBUG_UNREACHABLE(detail::assert_handler{});
        return cpp_cv_none;
    }

    std::unique_ptr<cpp_type> make_cv_qualified(std::unique_ptr<cpp_type> entity, cpp_cv cv)
    {
        if (cv == cpp_cv_none)
            return std::move(entity);
        return cpp_cv_qualified_type::build(std::move(entity), cv);
    }

    template <typename Builder>
    std::unique_ptr<cpp_type> make_leave_type(const CXType& type, Builder b)
    {
        auto spelling = get_type_spelling(type);

        // check for cv qualifiers on the leave type
        auto prefix = prefix_cv(spelling);
        auto suffix = suffix_cv(spelling);
        auto cv     = merge_cv(prefix, suffix);

        auto entity = b(std::move(spelling));
        return make_cv_qualified(std::move(entity), cv);
    }

    cpp_reference get_reference_kind(const CXType& type)
    {
        if (type.kind == CXType_LValueReference)
            return cpp_ref_lvalue;
        else if (type.kind == CXType_RValueReference)
            return cpp_ref_rvalue;
        return cpp_ref_none;
    }

    std::unique_ptr<cpp_type> parse_type_impl(const detail::parse_context& context,
                                              const CXType&                type)
    {
        switch (type.kind)
        {
        // stuff I can't parse
        // or have no idea what it is and wait for bug report
        case CXType_Invalid:
        case CXType_Overload:
        case CXType_ObjCId:
        case CXType_ObjCClass:
        case CXType_ObjCSel:
        case CXType_Complex:
        case CXType_BlockPointer:
        case CXType_Vector:
        case CXType_ObjCInterface:
        case CXType_ObjCObjectPointer:
        {
            auto msg = detail::format("unexpected type of kind '",
                                      detail::get_type_kind_spelling(type).c_str(), "'");
            auto location = source_location::make(get_type_spelling(type).c_str());
            context.logger->log("libclang parser", diagnostic{msg, location, severity::warning});
        }
        // fallthrough
        case CXType_Unexposed:
            return cpp_unexposed_type::build(get_type_spelling(type).c_str());

        case CXType_Void:
        case CXType_Bool:
        case CXType_Char_U:
        case CXType_UChar:
        case CXType_Char16:
        case CXType_Char32:
        case CXType_UShort:
        case CXType_UInt:
        case CXType_ULong:
        case CXType_ULongLong:
        case CXType_UInt128:
        case CXType_Char_S:
        case CXType_SChar:
        case CXType_WChar:
        case CXType_Short:
        case CXType_Int:
        case CXType_Long:
        case CXType_LongLong:
        case CXType_Int128:
        case CXType_Float:
        case CXType_Double:
        case CXType_LongDouble:
        case CXType_NullPtr:
        case CXType_Float128:
            return make_leave_type(type, [](std::string&& spelling) {
                return cpp_builtin_type::build(std::move(spelling));
            });

        case CXType_Record:
        case CXType_Enum:
        case CXType_Typedef:
        case CXType_Elaborated:
            return make_leave_type(type, [&](std::string&& spelling) {
                auto decl = clang_getTypeDeclaration(type);
                return cpp_user_defined_type::build(
                    cpp_type_ref(detail::get_entity_id(decl), std::move(spelling)));
            });

        case CXType_Pointer:
        {
            auto pointee = parse_type_impl(context, clang_getPointeeType(type));
            auto pointer = cpp_pointer_type::build(std::move(pointee));

            auto spelling = get_type_spelling(type);
            auto cv       = suffix_cv(spelling);
            return make_cv_qualified(std::move(pointer), cv);
        }
        case CXType_LValueReference:
        case CXType_RValueReference:
        {
            auto referee = parse_type_impl(context, clang_getPointeeType(type));
            return cpp_reference_type::build(std::move(referee), get_reference_kind(type));
        }

        // TODO
        case CXType_IncompleteArray:
        case CXType_VariableArray:
        case CXType_DependentSizedArray:
        case CXType_ConstantArray:
        {
            break;
        }

        case CXType_Dependent:
            break;

        case CXType_FunctionNoProto:
            break;
        case CXType_FunctionProto:
            break;
        case CXType_MemberPointer:
            break;

        case CXType_Auto:
            break;
        }

        return nullptr;
    }
}

std::unique_ptr<cpp_type> detail::parse_type(const detail::parse_context& context,
                                             const CXType&                type)
{
    auto result = parse_type_impl(context, type);
    DEBUG_ASSERT(result && is_valid(*result), detail::parse_error_handler{}, type,
                 "invalid type parsed");
    return std::move(result);
}

std::unique_ptr<cpp_entity> detail::parse_cpp_type_alias(const detail::parse_context& context,
                                                         const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_TypeAliasDecl, detail::assert_handler{});

    detail::tokenizer    tokenizer(context.tu, context.file, cur);
    detail::token_stream stream(tokenizer, cur);

    // using <identifier> = ...
    detail::skip(stream, "using");
    auto& name = stream.get().value();
    detail::skip(stream, "=");

    auto type = parse_type(context, clang_getTypedefDeclUnderlyingType(cur));
    return cpp_type_alias::build(*context.idx, get_entity_id(cur), name.c_str(), std::move(type));
}
