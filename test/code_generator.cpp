// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/code_generator.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("code_generator")
{
    SECTION("basic")
    {
        // no need to check much here, as each entity check separately
        // only write some file with equivalent code and synopsis
        auto code = R"(using type=int;

struct foo{
  int a;

  auto func(int)->int(*(*)(int))[42];

private:
  int const b=42;
};

int(*(*(foo::* mptr)(int))(int))[42];

enum class bar
:int{
  a,
  b=42
};

void func(int(*)(int));

extern void(* ptr)(int(*)(int))=&func;
)";

        auto file = parse({}, "code_generator.cpp", code);
        REQUIRE(get_code(*file) == code);
    }
    SECTION("exclude target")
    {
        auto code = R"(
namespace a {}

namespace b = a;

using c = int*;
typedef int d;
)";

        auto synopsis = R"(namespace a{
}

namespace b=excluded;

using c=excluded;

using d=excluded;
)";

        auto file = parse({}, "code_generator_exclude_target.cpp", code);
        REQUIRE(get_code(*file, code_generator::exclude_target) == synopsis);
    }
    SECTION("exclude return")
    {
        auto code = R"(
void a();
template <typename T>
auto b() -> int*;

struct foo
{
    int c() const&;
    operator const int ();
};
)";

        auto synopsis = R"(excluded a();

template<typename T>
excluded b();

struct foo{
  excluded c()const&;

  operator excluded();
};
)";

        auto file = parse({}, "code_generator_exclude_return.cpp", code);
        REQUIRE(get_code(*file, code_generator::exclude_return) == synopsis);
    }
}
