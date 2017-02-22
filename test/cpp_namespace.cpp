// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_namespace.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_namespace")
{
    auto code = R"(
namespace a {}

inline namespace b {}

namespace c
{
    namespace d {}
}
)";

    auto file  = parse({}, "cpp_namespace.cpp", code);
    auto count = test_visit<cpp_namespace>(*file, [&](const cpp_namespace& ns) {
        auto no_children = count_children(ns);
        if (ns.name() == "a")
        {
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "b")
        {
            REQUIRE(ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else if (ns.name() == "c")
        {
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 1u);
        }
        else if (ns.name() == "d")
        {
            check_parent(ns, "c", "c::d");
            REQUIRE(!ns.is_inline());
            REQUIRE(no_children == 0u);
        }
        else
            REQUIRE(false);
    });
    REQUIRE(count == 4u);
}

TEST_CASE("cpp_namespace_alias")
{
    auto code = R"(
namespace ns1 {}
namespace ns2 {}

namespace a = ns1;
namespace b = ns2;

namespace outer
{
    namespace ns {}

    namespace c = ns;
    namespace d = ns1;
}

namespace e = outer::ns;
namespace f = outer::c;
)";

    cpp_entity_index idx;
    auto check_alias = [&](const cpp_namespace_alias& alias, const char* target_name,
                           const char* target_full_name) {
        auto& target = alias.target();
        REQUIRE(target.name() == target_name);

        auto entity = target.get(idx);
        REQUIRE(entity);
        REQUIRE(full_name(entity.value()) == target_full_name);
    };

    auto file  = parse(idx, "cpp_namespace_alias.cpp", code);
    auto count = test_visit<cpp_namespace_alias>(*file, [&](const cpp_namespace_alias& alias) {
        if (alias.name() == "a")
            check_alias(alias, "ns1", "ns1");
        else if (alias.name() == "b")
            check_alias(alias, "ns2", "ns2");
        else if (alias.name() == "c")
        {
            check_parent(alias, "outer", "outer::c");
            check_alias(alias, "ns", "outer::ns");
        }
        else if (alias.name() == "d")
        {
            check_parent(alias, "outer", "outer::d");
            check_alias(alias, "ns1", "ns1");
        }
        else if (alias.name() == "e")
            check_alias(alias, "outer::ns", "outer::ns");
        else if (alias.name() == "f")
            check_alias(alias, "outer::c", "outer::ns");
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

using namespace ns1;
using namespace ns2;

namespace outer
{
    namespace ns {}

    using namespace ns;
    using namespace ::ns1;
}

using namespace outer::ns;
)";

    cpp_entity_index idx;
    auto check_directive = [&](const cpp_using_directive& directive, const char* target_full_name) {
        auto target = directive.target();

        auto entity = target.get(idx);
        REQUIRE(entity);
        REQUIRE(full_name(entity.value()) == target_full_name);
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
    // TODO: write test when actual entities can be parsed
}
