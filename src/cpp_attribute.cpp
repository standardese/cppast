// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_attribute.hpp>

#include <algorithm>

#include <cppast/cpp_entity.hpp>

using namespace cppast;

namespace
{
const char* get_attribute_name(cpp_attribute_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_attribute_kind::alignas_:
        return "alignas";
    case cpp_attribute_kind::carries_dependency:
        return "carries_dependency";
    case cpp_attribute_kind::deprecated:
        return "deprecated";
    case cpp_attribute_kind::fallthrough:
        return "fallthrough";
    case cpp_attribute_kind::maybe_unused:
        return "maybe_unused";
    case cpp_attribute_kind::nodiscard:
        return "nodiscard";
    case cpp_attribute_kind::noreturn:
        return "noreturn";

    case cpp_attribute_kind::unknown:
        return "unknown";
    }

    return "<error>";
}
} // namespace

cpp_attribute::cpp_attribute(cpp_attribute_kind                    kind,
                             type_safe::optional<cpp_token_string> arguments)
: cpp_attribute(type_safe::nullopt, get_attribute_name(kind), std::move(arguments), false)
{
    kind_ = kind;
}

type_safe::optional_ref<const cpp_attribute> cppast::has_attribute(
    const cpp_attribute_list& attributes, const std::string& name)
{
    auto iter
        = std::find_if(attributes.begin(), attributes.end(), [&](const cpp_attribute& attribute) {
              if (attribute.scope())
                  return attribute.scope().value() + "::" + attribute.name() == name;
              else
                  return attribute.name() == name;
          });

    if (iter == attributes.end())
        return nullptr;
    else
        return type_safe::ref(*iter);
}

type_safe::optional_ref<const cpp_attribute> cppast::has_attribute(
    const cpp_attribute_list& attributes, cpp_attribute_kind kind)
{
    auto iter
        = std::find_if(attributes.begin(), attributes.end(),
                       [&](const cpp_attribute& attribute) { return attribute.kind() == kind; });

    if (iter == attributes.end())
        return nullptr;
    else
        return type_safe::ref(*iter);
}

type_safe::optional_ref<const cpp_attribute> cppast::has_attribute(const cpp_entity&  e,
                                                                   const std::string& name)
{
    return has_attribute(e.attributes(), name);
}

type_safe::optional_ref<const cpp_attribute> cppast::has_attribute(const cpp_entity&  e,
                                                                   cpp_attribute_kind kind)
{
    return has_attribute(e.attributes(), kind);
}
