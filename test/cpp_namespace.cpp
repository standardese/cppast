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
