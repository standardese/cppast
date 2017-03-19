// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
#define CPPAST_CPP_ENTITY_REF_HPP_INCLUDED

#include <vector>

#include <type_safe/variant.hpp>

#include <cppast/cpp_entity_index.hpp>

namespace cppast
{
    enum class cpp_entity_kind;

    /// A basic reference to some kind of [cppast::cpp_entity]().
    ///
    /// It can either refer to a single [cppast::cpp_entity]()
    /// or multiple.
    /// In the later case it is *overloaded*.
    template <typename T, typename Predicate>
    class basic_cpp_entity_ref
    {
    public:
        /// \effects Creates it giving it the target id and name.
        basic_cpp_entity_ref(cpp_entity_id target_id, std::string target_name)
        : target_(std::move(target_id)), name_(std::move(target_name))
        {
        }

        /// \effects Creates it giving it multiple target ids and name.
        /// \notes This is to refer to an overloaded function.
        basic_cpp_entity_ref(std::vector<cpp_entity_id> target_ids, std::string target_name)
        : target_(std::move(target_ids)), name_(std::move(target_name))
        {
        }

        /// \returns The name of the reference, as spelled in the source code.
        const std::string& name() const noexcept
        {
            return name_;
        }

        /// \returns Whether or not it refers to multiple entities.
        bool is_overloaded() const noexcept
        {
            return target_.has_value(type_safe::variant_type<std::vector<cpp_entity_id>>{});
        }

        /// \returns The number of entities it refers to.
        type_safe::size_t no_overloaded() const noexcept
        {
            return id().size();
        }

        /// \returns An array reference to the id or ids it refers to.
        type_safe::array_ref<const cpp_entity_id> id() const noexcept
        {
            if (is_overloaded())
            {
                auto& vec = target_.value(type_safe::variant_type<std::vector<cpp_entity_id>>{});
                return type_safe::ref(vec.data(), vec.size());
            }
            else
            {
                auto& id = target_.value(type_safe::variant_type<cpp_entity_id>{});
                return type_safe::ref(&id, 1u);
            }
        }

    private:
        class ref_impl
        {
        public:
            ref_impl(type_safe::object_ref<const cpp_entity_index> idx,
                     type_safe::array_ref<const cpp_entity_id>     ids)
            : ids_(ids), index_(idx)
            {
            }

            class iterator
            {
            public:
                using value_type        = type_safe::optional_ref<const T>;
                using reference         = value_type;
                using pointer           = value_type*;
                using difference_type   = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                reference operator*() const noexcept
                {
                    return (*ref_)[i_];
                }

                pointer operator->() const noexcept
                {
                    return &(*ref_)[i_];
                }

                iterator& operator++() noexcept
                {
                    ++i_;
                    return *this;
                }

                iterator operator++(int)noexcept
                {
                    auto tmp = *this;
                    ++(*this);
                    return tmp;
                }

                friend bool operator==(const iterator& a, const iterator& b) noexcept
                {
                    return a.ref_ == b.ref_ && a.i_ == b.i_;
                }

                friend bool operator!=(const iterator& a, const iterator& b) noexcept
                {
                    return !(a == b);
                }

            private:
                iterator(type_safe::object_ref<const ref_impl> ref, type_safe::index_t i)
                : ref_(ref), i_(i)
                {
                }

                type_safe::object_ref<const ref_impl> ref_;
                type_safe::index_t                    i_;

                friend ref_impl;
            };

            iterator begin() const noexcept
            {
                return iterator(type_safe::ref(*this), 0u);
            }

            iterator end() const noexcept
            {
                return iterator(type_safe::ref(*this), size());
            }

            type_safe::optional_ref<const T> operator[](type_safe::index_t i) const
            {
                return index_->lookup(ids_[i]).map([](const cpp_entity& e) -> const T& {
                    DEBUG_ASSERT(Predicate{}(e), detail::precondition_error_handler{},
                                 "predicate not fulfilled");
                    return static_cast<const T&>(e);
                });
            }

            type_safe::size_t size() const noexcept
            {
                return ids_.size();
            }

        private:
            type_safe::array_ref<const cpp_entity_id>     ids_;
            type_safe::object_ref<const cpp_entity_index> index_;
        };

    public:
        /// \returns An array reference to the entities it refers to.
        /// The return type provides `operator[]` + `size()`,
        /// as well as `begin()` and `end()` returning forward iterators.
        /// \exclude return
        ref_impl get(const cpp_entity_index& idx) const noexcept
        {
            return ref_impl(type_safe::ref(idx), id());
        }

    private:
        type_safe::variant<cpp_entity_id, std::vector<cpp_entity_id>> target_;
        std::string name_;
    };

    /// \exclude
    namespace detail
    {
        struct cpp_entity_ref_predicate
        {
            bool operator()(const cpp_entity&)
            {
                return true;
            }
        };
    }

    /// A [cppast::basic_cpp_entity_ref]() to any [cppast::cpp_entity]().
    using cpp_entity_ref = basic_cpp_entity_ref<cpp_entity, detail::cpp_entity_ref_predicate>;
} // namespace cppast

#endif // CPPAST_CPP_ENTITY_REF_HPP_INCLUDED
