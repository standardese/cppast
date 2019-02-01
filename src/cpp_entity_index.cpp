// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_entity_index.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/detail/assert.hpp>

using namespace cppast;

cpp_entity_index::duplicate_definition_error::duplicate_definition_error()
: std::logic_error("duplicate registration of entity definition")
{}

void cpp_entity_index::register_definition(cpp_entity_id                           id,
                                           type_safe::object_ref<const cpp_entity> entity) const
{
    DEBUG_ASSERT(entity->kind() != cpp_entity_kind::namespace_t,
                 detail::precondition_error_handler{}, "must not be a namespace");
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        result = map_.emplace(std::move(id), value(entity, true));
    if (!result.second)
    {
        // already in map, override declaration
        auto& value = result.first->second;
        if (value.is_definition && !is_template_specialization(value.entity->kind()))
            // allow duplicate definition of template specializations as a workaround for MacOS
            throw duplicate_definition_error();
        value.is_definition = true;
        value.entity        = entity;
    }
}

bool cpp_entity_index::register_file(cpp_entity_id                         id,
                                     type_safe::object_ref<const cpp_file> file) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return map_.emplace(std::move(id), value(file, true)).second;
}

void cpp_entity_index::register_forward_declaration(
    cpp_entity_id id, type_safe::object_ref<const cpp_entity> entity) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    map_.emplace(std::move(id), value(entity, false));
}

void cpp_entity_index::register_namespace(cpp_entity_id                              id,
                                          type_safe::object_ref<const cpp_namespace> ns) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    ns_[std::move(id)].push_back(ns);
}

type_safe::optional_ref<const cpp_entity> cpp_entity_index::lookup(const cpp_entity_id& id) const
    noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = map_.find(id);
    if (iter == map_.end())
        return {};
    return type_safe::ref(iter->second.entity.get());
}

type_safe::optional_ref<const cpp_entity> cpp_entity_index::lookup_definition(
    const cpp_entity_id& id) const noexcept
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = map_.find(id);
    if (iter == map_.end() || !iter->second.is_definition)
        return {};
    return type_safe::ref(iter->second.entity.get());
}

auto cpp_entity_index::lookup_namespace(const cpp_entity_id& id) const noexcept
    -> type_safe::array_ref<type_safe::object_ref<const cpp_namespace>>
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto                        iter = ns_.find(id);
    if (iter == ns_.end())
        return nullptr;
    auto& vec = iter->second;
    return type_safe::ref(vec.data(), vec.size());
}
