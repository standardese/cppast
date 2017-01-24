// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_class.hpp>

#include <cppast/cpp_entity_index.hpp>
#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;

std::unique_ptr<cpp_class> cpp_class::builder::finish(const cpp_entity_index& idx,
                                                      cpp_entity_id           id) noexcept
{
    idx.register_entity(std::move(id), type_safe::ref(*class_));
    return std::move(class_);
}

const char* cppast::to_string(cpp_class_kind kind) noexcept
{
    switch (kind)
    {
    case cpp_class_kind::class_t:
        return "class";
    case cpp_class_kind::struct_t:
        return "struct";
    case cpp_class_kind::union_t:
        return "union";
    }

    return "should not get here";
}

const char* cppast::to_string(cpp_access_specifier_kind access) noexcept
{
    switch (access)
    {
    case cpp_public:
        return "public";
    case cpp_protected:
        return "protected";
    case cpp_private:
        return "private";
    }

    return "should not get here either";
}

cpp_entity_kind cpp_access_specifier::do_get_entity_kind() const noexcept
{
    return cpp_entity_kind::access_specifier_t;
}

cpp_entity_kind cpp_class::do_get_entity_kind() const noexcept
{
    return cpp_entity_kind::class_t;
}
