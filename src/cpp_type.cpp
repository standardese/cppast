// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_type.hpp>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_function_type.hpp>

using namespace cppast;

bool detail::cpp_type_ref_predicate::operator()(const cpp_entity& e)
{
    return is_type(e.kind());
}

namespace
{
    bool can_compose(const cpp_type& type)
    {
        return type.kind() != cpp_type_kind::function
               && type.kind() != cpp_type_kind::member_function
               && type.kind() != cpp_type_kind::member_object;
    }
}

bool cppast::is_valid(const cpp_type& type) noexcept
{
    switch (type.kind())
    {
    case cpp_type_kind::cv_qualified:
    {
        auto& qual = static_cast<const cpp_cv_qualified_type&>(type);
        if (qual.type().kind() == cpp_type_kind::reference)
            return false; // not allowed
        return is_valid(qual.type());
    }
    case cpp_type_kind::pointer:
    {
        auto& ptr = static_cast<const cpp_pointer_type&>(type);
        if (ptr.pointee().kind() == cpp_type_kind::reference)
            return false; // not allowed
        return is_valid(ptr.pointee());
    }
    case cpp_type_kind::reference:
    {
        auto& ref = static_cast<const cpp_reference_type&>(type);
        if (ref.referee().kind() == cpp_type_kind::member_function
            || ref.referee().kind() == cpp_type_kind::member_object)
            return false; // not allowed
        return is_valid(ref.referee());
    }

    case cpp_type_kind::array:
    {
        auto& array = static_cast<const cpp_array_type&>(type);
        if (array.value_type().kind() == cpp_type_kind::reference
            || !can_compose(array.value_type()))
            return false;
        return is_valid(array.value_type());
    }

    case cpp_type_kind::function:
    {
        auto& func = static_cast<const cpp_function_type&>(type);

        if (!can_compose(func.return_type()) || !is_valid(func.return_type()))
            return false;

        for (auto& arg : func.parameter_types())
            if (!can_compose(arg) || !is_valid(arg))
                return false;

        return true;
    }
    case cpp_type_kind::member_function:
    {
        auto& func = static_cast<const cpp_member_function_type&>(type);

        if (func.class_type().kind() != cpp_type_kind::user_defined)
            return false;
        else if (!can_compose(func.return_type()) || !is_valid(func.return_type()))
            return false;

        for (auto& arg : func.parameter_types())
            if (!can_compose(arg) || !is_valid(arg))
                return false;

        return true;
    }
    case cpp_type_kind::member_object:
    {
        auto& obj = static_cast<const cpp_member_object_type&>(type);

        if (obj.class_type().kind() != cpp_type_kind::user_defined)
            return false;
        return is_valid(obj.object_type());
    }

    case cpp_type_kind::builtin:
    case cpp_type_kind::user_defined:
    case cpp_type_kind::template_parameter:
    case cpp_type_kind::template_instantiation:
    case cpp_type_kind::unexposed:
        // no further check required/possible
        break;
    }

    return true;
}
