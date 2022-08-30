// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_concept") 
{
    auto code = R"(
#include <concepts>

template<typename T>
class container{};

template<typename T>
concept integer_direct = std::same_as<T, int>;

template<typename T>
concept has_methods = requires (T t)
{
    {t.a()} -> std::copy_constructible;
    {t.b()} -> integer_direct;
    {t.c()} -> std::convertible_to<int>;
};

template<typename T>
concept has_types = requires
{
  typename T::inner;
  typename container<T>;
};

template<typename T>
    requires integer_direct<T>
void f1(T param);

template<has_methods T>
class c {};

template<std::convertible_to<int> T>
void f2(T param);

)";
    cpp_entity_index idx;
    auto             file = parse(idx, "cpp_concept.cpp", code, false, cppast::cpp_standard::cpp_20);
    //TODO: actual tests


}