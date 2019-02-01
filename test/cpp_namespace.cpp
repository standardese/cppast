// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_namespace.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_namespace")
{
    auto code = R"(
/// namespace a{
/// }
namespace a {}

/// inline namespace b{
/// }
inline namespace b {}

/// namespace c{
///   namespace d{
///   }
/// }
namespace c
{
    /// namespace d{
    /// }
    namespace d {}
}

/// namespace {
/// }
namespace {}

/// namespace f{
/// }
namespace e::f {}

// unnecessary semicolon at end of file
;
)";

    auto file  = parse({}, "cpp_namespace.cpp", code);
    auto count = test_visit<cpp_namespace>(*file, [&](const cpp_namespace& ns) {
        auto no_children = count_children(ns);
        if (ns.name() == "a")
        {
            REQUIRE(!ns.is_anonymous());
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "b")
        {
            REQUIRE(!ns.is_anonymous());
            REQUIRE(ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "c")
        {
            REQUIRE(!ns.is_anonymous());
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 1u);
        }
        else if (ns.name() == "d")
        {
            REQUIRE(!ns.is_anonymous());
            check_parent(ns, "c", "c::d");
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "")
        {
            REQUIRE(ns.is_anonymous());
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "e")
        {
            REQUIRE(!ns.is_anonymous());
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 1u);
            return false; // don't have a comment
        }
        else if (ns.name() == "f")
        {
            REQUIRE(!ns.is_anonymous());
            check_parent(ns, "e", "e::f");
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else
            REQUIRE(false);

        return true;
    });
    REQUIRE(count == 7u);
}

TEST_CASE("cpp_namespace_alias")
{
    auto code = R"(
namespace outer {}
namespace ns {}

/// namespace a=outer;
namespace a = outer;
/// namespace b=ns;
namespace b = ns;

namespace outer
{
    namespace ns {}

    /// namespace c=ns;
    namespace c = ns;
    /// namespace d=::outer;
    namespace d = ::outer;
}

/// namespace e=outer::ns;
namespace e = outer::ns;
/// namespace f=outer::c;
namespace f = outer::c;
)";

    cpp_entity_index idx;
    auto             check_alias = [&](const cpp_namespace_alias& alias, const char* target_name,
                           const char* target_full_name, unsigned no) {
        auto& target = alias.target();
        REQUIRE(target.name() == target_name);
        REQUIRE(!target.is_overloaded());

        auto entities = target.get(idx);
        REQUIRE(entities.size() == no);
        for (auto& entity : entities)
            REQUIRE(full_name(*entity) == target_full_name);
    };

    auto file  = parse(idx, "cpp_namespace_alias.cpp", code);
    auto count = test_visit<cpp_namespace_alias>(*file, [&](const cpp_namespace_alias& alias) {
        if (alias.name() == "a")
            check_alias(alias, "outer", "outer", 2u);
        else if (alias.name() == "b")
            check_alias(alias, "ns", "ns", 1u);
        else if (alias.name() == "c")
        {
            check_parent(alias, "outer", "outer::c");
            check_alias(alias, "ns", "outer::ns", 1u);
        }
        else if (alias.name() == "d")
        {
            check_parent(alias, "outer", "outer::d");
            check_alias(alias, "::outer", "outer", 2u);
        }
        else if (alias.name() == "e")
            check_alias(alias, "outer::ns", "outer::ns", 1u);
        else if (alias.name() == "f")
            check_alias(alias, "outer::c", "outer::ns", 1u);
        else
            REQUIRE(false);
    });
    REQUIRE(count == 6u);
}

TEST_CASE("cpp_using_directive")
{
    auto code = R"(
namespace ns1 {}
namespace ns2 {}

/// using namespace ns1;
using namespace ns1;
/// using namespace ns2;
using namespace ns2;

namespace outer
{
    namespace ns {}

    /// using namespace ns;
    using namespace ns;
    /// using namespace ::ns1;
    using namespace ::ns1;
}

/// using namespace outer::ns;
using namespace outer::ns;
)";

    cpp_entity_index idx;
    auto check_directive = [&](const cpp_using_directive& directive, const char* target_full_name) {
        auto target = directive.target();
        REQUIRE(!target.is_overloaded());

        auto entities = target.get(idx);
        REQUIRE(entities.size() == 1u);
        REQUIRE(full_name(*entities[0u]) == target_full_name);
    };

    auto file  = parse(idx, "cpp_using_directive.cpp", code);
    auto count = test_visit<cpp_using_directive>(*file, [&](const cpp_using_directive& directive) {
        if (directive.target().name() == "ns1")
            check_directive(directive, "ns1");
        else if (directive.target().name() == "ns2")
            check_directive(directive, "ns2");
        else if (directive.target().name() == "ns")
        {
            check_parent(directive, "outer", "");
            check_directive(directive, "outer::ns");
        }
        else if (directive.target().name() == "::ns1")
        {
            check_parent(directive, "outer", "");
            check_directive(directive, "ns1");
        }
        else if (directive.target().name() == "outer::ns")
            check_directive(directive, "outer::ns");
        else
            REQUIRE(false);
    });
    REQUIRE(count == 5u);
}

TEST_CASE("cpp_using_declaration")
{
    auto code = R"(
namespace ns1
{
    enum a {};
    enum b {};
}

namespace ns2
{
    enum a {};
    enum b {};
}

/// using ns1::a;
using ns1::a;
/// using ns2::b;
using ns2::b;

namespace outer
{
    namespace ns
    {
        enum c {};
    }

    /// using ns::c;
    using ns::c;
}

/// using outer::ns::c;
using outer::ns::c;
/// using outer::c;
using outer::c;

namespace ns1
{
    void d(int);
    void d(float);
}

/// using ns1::d;
using ns1::d;
)";

    cpp_entity_index idx;
    auto             check_declaration
        = [&](const cpp_using_declaration& decl, const char* target_full_name, unsigned no) {
              auto target = decl.target();
              REQUIRE((target.no_overloaded() == no));
              for (auto entity : target.get(idx))
              {
                  REQUIRE(full_name(*entity) == target_full_name);
              }
          };

    auto file  = parse(idx, "cpp_using_declaration.cpp", code);
    auto count = test_visit<cpp_using_declaration>(*file, [&](const cpp_using_declaration& decl) {
        REQUIRE(decl.name().empty());

        if (decl.target().name() == "ns1::a")
            check_declaration(decl, "ns1::a", 1u);
        else if (decl.target().name() == "ns2::b")
            check_declaration(decl, "ns2::b", 1u);
        else if (decl.target().name() == "ns::c")
        {
            check_parent(decl, "outer", "");
            check_declaration(decl, "outer::ns::c", 1u);
        }
        else if (decl.target().name() == "outer::ns::c")
            check_declaration(decl, "outer::ns::c", 1u);
        else if (decl.target().name() == "outer::c")
            check_declaration(decl, "outer::ns::c", 1u);
        else if (decl.target().name() == "ns1::d")
            check_declaration(decl, "ns1::d", 2u);
        else
            REQUIRE(false);
    });
    REQUIRE(count == 6u);
}
