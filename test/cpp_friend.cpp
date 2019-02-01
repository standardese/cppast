// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_friend.hpp>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_class_template.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_function_template.hpp>
#include <cppast/cpp_template.hpp>
#include <cppast/cpp_template_parameter.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_friend", "[!hide][clang4]")
{
    auto code = R"(
/// b
class b
{
public:
    /// b::b
    b(const b&) {}

    void f() const;

    /// operator int
    operator int() {}
};

/// g
class g {};

namespace ns
{
    namespace other_ns
    {
        namespace inner
        {
            /// h2
            void h2() {}
        }
    }

    /// h
    class h
    {
         /// friend void other_ns::inner::h2();
         friend void other_ns::inner::h2();
    };
}

/// b::f
void b::f() const {}

struct foo
{
    /// friend struct a;
    friend struct a;
    /// friend class b;
    friend class b;

    /// friend void c(int);
    friend void c(int);
    /// friend int d();
    friend int d();
    /// friend void e();
    friend void e() {}

    /// friend b::b(b const&);
    friend b::b(const b&);
    /// friend void b::f()const;
    friend void b::f() const;
    /// friend b::operator int();
    friend b::operator int();

    /// friend class g;
    friend g;
    /// friend ns::h;
    friend ns::h;
};

template <typename T>
struct templ_a {};

namespace ns
{
    template <typename T, int I, template <typename> class Templ>
    struct templ_b {};
}

template <typename T>
void templ_c();

template <typename T>
struct bar
{
    /// friend T;
    friend T;
    /// friend templ_a<T>;
    friend templ_a<T>;
    /// friend ns::templ_b<T,0,templ_a>;
    friend struct ns::templ_b<T, 0, templ_a>;

    /// template<typename U>
    /// friend struct i;
    template <typename U>
    friend struct i;

    /// template<typename U>
    /// friend void j();
    template <typename U>
    friend void j();
    /// template<typename U>
    /// friend void k();
    template <typename U>
    friend void k() {}

    /// friend void templ_c<ns::h>();
    friend void templ_c<ns::h>();
};

/// d
int d() {}
)";

    cpp_entity_index idx;
    auto             check_definition = [&](cpp_entity_id id, const char* name) {
        auto def = idx.lookup_definition(id);
        REQUIRE(def.has_value());
        REQUIRE(def.value().comment());
        REQUIRE(def.value().comment().value() == name);
    };

    auto file  = parse(idx, "cpp_friend.cpp", code);
    auto count = test_visit<cpp_friend>(*file, [&](const cpp_friend& f) {
        REQUIRE(f.name().empty());

        if (auto entity = f.entity())
        {
            if (entity.value().kind() == cpp_entity_kind::class_t)
            {
                auto& c = static_cast<const cpp_class&>(entity.value());
                REQUIRE(c.is_declaration());
                if (c.name() == "a")
                    REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
                else if (c.name() == "b")
                {
                    REQUIRE(c.class_kind() == cpp_class_kind::class_t);
                    REQUIRE(c.definition());
                    check_definition(c.definition().value(), "b");
                }
                else if (c.name() == "g")
                {
                    REQUIRE(c.class_kind() == cpp_class_kind::class_t);
                    REQUIRE(c.definition());
                    check_definition(c.definition().value(), "g");
                }
                else
                    REQUIRE(false);
            }
            else if (entity.value().kind() == cpp_entity_kind::class_template_t)
            {
                auto& c = static_cast<const cpp_class_template&>(entity.value());
                if (c.name() == "i")
                    REQUIRE(c.class_().class_kind() == cpp_class_kind::struct_t);
                else
                    REQUIRE(false);
            }
            else if (is_function(entity.value().kind()))
            {
                auto& func = static_cast<const cpp_function_base&>(entity.value());
                if (func.name() == "c")
                    REQUIRE(func.is_declaration());
                else if (func.name() == "d")
                {
                    REQUIRE(func.is_declaration());
                    REQUIRE(func.definition());
                    check_definition(func.definition().value(), "d");
                }
                else if (func.name() == "e")
                    REQUIRE(func.is_definition());
                else if (func.name() == "b")
                {
                    REQUIRE(func.semantic_scope() == "b::");
                    REQUIRE(func.is_declaration());
                    REQUIRE(func.definition());
                    check_definition(func.definition().value(), "b::b");
                }
                else if (func.name() == "f")
                {
                    REQUIRE(func.semantic_scope() == "b::");
                    REQUIRE(func.is_declaration());
                    REQUIRE(func.definition());
                    check_definition(func.definition().value(), "b::f");
                }
                else if (func.name() == "h2")
                {
                    REQUIRE(func.semantic_scope() == "other_ns::inner::");
                    REQUIRE(func.is_declaration());
                    REQUIRE(func.definition());
                    check_definition(func.definition().value(), "h2");
                }
                else if (func.name() == "operator int")
                {
                    REQUIRE(func.semantic_scope() == "b::");
                    REQUIRE(func.is_declaration());
                    REQUIRE(func.definition());
                    check_definition(func.definition().value(), "operator int");
                }
                else
                    REQUIRE(false);
            }
            else if (entity.value().kind() == cpp_entity_kind::function_template_t)
            {
                auto& func = static_cast<const cpp_function_template&>(entity.value());
                if (func.name() == "j")
                    REQUIRE(func.function().is_declaration());
                else if (func.name() == "k")
                    REQUIRE(func.function().is_definition());
                else
                    REQUIRE(false);
            }
            else if (entity.value().kind() == cpp_entity_kind::function_template_specialization_t)
            {
                auto& func
                    = static_cast<const cpp_function_template_specialization&>(entity.value());
                if (func.name() == "templ_c")
                {
                    REQUIRE(func.function().is_declaration());
                    REQUIRE(!func.arguments_exposed());
                    REQUIRE(func.unexposed_arguments().as_string() == "ns::h");
                }
                else
                    REQUIRE(false);
            }
            else
                REQUIRE(false);
        }
        else if (auto type = f.type())
        {
            if (type.value().kind() == cpp_type_kind::user_defined_t)
            {
                auto& user = static_cast<const cpp_user_defined_type&>(type.value());
                REQUIRE(!user.entity().is_overloaded());
                if (user.entity().name() == "ns::h")
                    check_definition(user.entity().id()[0u], "h");
                else
                    REQUIRE(false);
            }
            else if (type.value().kind() == cpp_type_kind::template_parameter_t)
            {
                auto& param = static_cast<const cpp_template_parameter_type&>(type.value());
                REQUIRE(!param.entity().is_overloaded());
                if (param.entity().name() == "T")
                    REQUIRE(
                        equal_types(idx, param,
                                    *cpp_template_parameter_type::build(
                                        cpp_template_type_parameter_ref(cpp_entity_id(""), "T"))));
                else
                    REQUIRE(false);
            }
            else if (type.value().kind() == cpp_type_kind::template_instantiation_t)
            {
                auto& inst = static_cast<const cpp_template_instantiation_type&>(type.value());
                cpp_template_instantiation_type::builder builder(
                    cpp_template_ref(cpp_entity_id(""), inst.primary_template().name()));
                builder.add_argument(cpp_template_parameter_type::build(
                    cpp_template_type_parameter_ref(cpp_entity_id(""), "T")));

                if (inst.primary_template().name() == "templ_a")
                    REQUIRE(equal_types(idx, inst, *builder.finish()));
                else if (inst.primary_template().name() == "ns::templ_b")
                {
                    builder.add_argument(
                        cpp_literal_expression::build(cpp_builtin_type::build(cpp_int), "0"));
                    builder.add_argument(cpp_template_ref(cpp_entity_id(""), "templ_a"));
                    REQUIRE(equal_types(idx, inst, *builder.finish()));
                }
                else
                    REQUIRE(false);
            }
            else
                REQUIRE(false);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 18u);
}
