// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/code_generator.hpp>

#include "test_parser.hpp"

TEST_CASE("code_generator")
{
    // no need to check much here, as each entity check separately
    // only write some file with equivalent code and synopsis
    auto code = R"(using type=int;

struct foo{
  int a;

  auto func(int)->int(*)[];

private:
  int const b=42;
};

int(*(foo::* mptr)(int))[];

enum class bar
:int{
  a,
  b=42
};

void func(int(*)(int));

extern void(* ptr)(int(*)(int))=&func;

template<typename T>int var;)";

    auto file = parse({}, "code_generator.cpp", code);
    REQUIRE(get_code(*file) == code);
}
