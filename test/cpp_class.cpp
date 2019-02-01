// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_class.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_class")
{
    auto code = R"(
// forward declarations
/// struct a;
struct a;
/// class b;
class b;
/// struct unresolved;
struct unresolved;

// basic
/// struct a{
/// };
struct a {};
/// class b final{
/// };
class b final {};
/// union c{
/// };
union c {};

// members
/// struct d{
///   enum m1{
///   };
///
///   enum m2{
///   };
///
/// private:
///   enum m3{
///   };
///
/// protected:
///   enum m4{
///   };
/// };
struct d
{
    enum m1 {};

public:
    enum m2 {};

private:
private:
    enum m3 {};

protected:
    enum m4 {};
};

// bases
/// class e
/// :a,d{
/// };
class e
: a, private d {};

namespace ns
{
    /// struct base;
    struct base;
}

/// struct ns::base{
/// };
struct ns::base {};

/// struct f
/// :ns::base,virtual protected e{
/// };
struct f
: public ns::base, virtual protected e
{};

using namespace ns;

/// struct g
/// :base{
/// };
struct g
: base {};
)";

    cpp_entity_index idx;
    auto             file  = parse(idx, "cpp_class.cpp", code);
    auto             count = test_visit<cpp_class>(*file, [&](const cpp_class& c) {
        if (c.name() == "a" || c.name() == "base")
        {
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.bases().begin() == c.bases().end());
            REQUIRE(c.begin() == c.end());

            auto definition = get_definition(idx, c);
            REQUIRE(definition);
            REQUIRE(definition.value().name() == c.name());
        }
        else if (c.name() == "b")
        {
            REQUIRE(c.class_kind() == cpp_class_kind::class_t);
            REQUIRE(c.bases().begin() == c.bases().end());
            REQUIRE(c.begin() == c.end());

            if (c.is_definition())
                REQUIRE(c.is_final());
            else
                REQUIRE(c.is_declaration());

            auto definition = get_definition(idx, c);
            REQUIRE(definition);
            REQUIRE(definition.value().name() == "b");
        }
        else if (c.name() == "unresolved")
        {
            REQUIRE(c.is_declaration());
            REQUIRE(!c.is_definition());
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.bases().begin() == c.bases().end());
            REQUIRE(c.begin() == c.end());

            auto definition = get_definition(idx, c);
            REQUIRE(!definition);
        }
        else if (c.name() == "c")
        {
            REQUIRE(c.is_definition());
            REQUIRE(!c.is_declaration());
            REQUIRE(c.class_kind() == cpp_class_kind::union_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.bases().begin() == c.bases().end());
            REQUIRE(c.begin() == c.end());
        }
        else if (c.name() == "d")
        {
            REQUIRE(c.is_definition());
            REQUIRE(!c.is_declaration());
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.bases().begin() == c.bases().end());

            auto no_children = 0u;
            for (auto& child : c)
            {
                switch (no_children++)
                {
                case 0:
                    REQUIRE(child.name() == "m1");
                    break;
                case 1:
                    REQUIRE(child.name() == "public");
                    REQUIRE(child.kind() == cpp_entity_kind::access_specifier_t);
                    REQUIRE(static_cast<const cpp_access_specifier&>(child).access_specifier()
                            == cpp_public);
                    break;
                case 2:
                    REQUIRE(child.name() == "m2");
                    break;
                case 3:
                    REQUIRE(child.name() == "private");
                    REQUIRE(child.kind() == cpp_entity_kind::access_specifier_t);
                    REQUIRE(static_cast<const cpp_access_specifier&>(child).access_specifier()
                            == cpp_private);
                    break;
                case 4:
                    REQUIRE(child.name() == "private");
                    REQUIRE(child.kind() == cpp_entity_kind::access_specifier_t);
                    REQUIRE(static_cast<const cpp_access_specifier&>(child).access_specifier()
                            == cpp_private);
                    break;
                case 5:
                    REQUIRE(child.name() == "m3");
                    break;
                case 6:
                    REQUIRE(child.name() == "protected");
                    REQUIRE(child.kind() == cpp_entity_kind::access_specifier_t);
                    REQUIRE(static_cast<const cpp_access_specifier&>(child).access_specifier()
                            == cpp_protected);
                    break;
                case 7:
                    REQUIRE(child.name() == "m4");
                    break;
                default:
                    REQUIRE(false);
                    break;
                }
            }
            REQUIRE(no_children == 8u);
        }
        else if (c.name() == "e")
        {
            REQUIRE(c.is_definition());
            REQUIRE(!c.is_declaration());
            REQUIRE(c.class_kind() == cpp_class_kind::class_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.begin() == c.end());

            auto no_bases = 0u;
            for (auto& base : c.bases())
            {
                ++no_bases;
                if (base.name() == "a")
                {
                    REQUIRE(base.access_specifier() == cpp_private);
                    REQUIRE(!base.is_virtual());

                    REQUIRE(equal_types(idx, base.type(),
                                        *cpp_user_defined_type::build(
                                            cpp_type_ref(cpp_entity_id(""), "a"))));
                }
                else if (base.name() == "d")
                {
                    REQUIRE(base.access_specifier() == cpp_private);
                    REQUIRE(!base.is_virtual());

                    REQUIRE(equal_types(idx, base.type(),
                                        *cpp_user_defined_type::build(
                                            cpp_type_ref(cpp_entity_id(""), "d"))));
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(no_bases == 2u);
        }
        else if (c.name() == "f")
        {
            REQUIRE(c.is_definition());
            REQUIRE(!c.is_declaration());
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.begin() == c.end());

            auto no_bases = 0u;
            for (auto& base : c.bases())
            {
                ++no_bases;
                if (base.name() == "ns::base")
                {
                    REQUIRE(base.access_specifier() == cpp_public);
                    REQUIRE(!base.is_virtual());

                    REQUIRE(equal_types(idx, base.type(),
                                        *cpp_user_defined_type::build(
                                            cpp_type_ref(cpp_entity_id(""), "ns::base"))));
                }
                else if (base.name() == "e")
                {
                    REQUIRE(base.access_specifier() == cpp_protected);
                    REQUIRE(base.is_virtual());

                    REQUIRE(equal_types(idx, base.type(),
                                        *cpp_user_defined_type::build(
                                            cpp_type_ref(cpp_entity_id(""), "e"))));
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(no_bases == 2u);
        }
        else if (c.name() == "g")
        {
            REQUIRE(c.is_definition());
            REQUIRE(!c.is_declaration());
            REQUIRE(c.class_kind() == cpp_class_kind::struct_t);
            REQUIRE(!c.is_final());
            REQUIRE(c.begin() == c.end());

            auto no_bases = 0u;
            for (auto& base : c.bases())
            {
                ++no_bases;
                if (base.name() == "base")
                {
                    REQUIRE(base.access_specifier() == cpp_public);
                    REQUIRE(!base.is_virtual());

                    REQUIRE(equal_types(idx, base.type(),
                                        *cpp_user_defined_type::build(
                                            cpp_type_ref(cpp_entity_id(""), "ns::base"))));
                }
                else
                    REQUIRE(false);
            }
            REQUIRE(no_bases == 1u);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 12u);
}
