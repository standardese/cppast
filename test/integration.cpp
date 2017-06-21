// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "test_parser.hpp"

#include <cppast/cpp_preprocessor.hpp>

using namespace cppast;

void parse_included_files(const cpp_entity_index& idx, const cpp_file& file)
{
    for (auto& e : file)
    {
        if (e.kind() == cpp_entity_kind::include_directive_t)
        {
            auto path = static_cast<const cpp_include_directive&>(e).full_path();
            parse_file(idx, path.c_str());
        }
    }
}

TEST_CASE("stdlib", "[!hide][integration]")
{
    auto code = R"(
// list of headers from: http://en.cppreference.com/w/cpp/header

//#include <cstdlib> -- problem with compiler built-in stuff on OSX
#include <csignal>
//#include <csetjmp> -- same as above
#include <cstdarg>
#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <bitset>
#include <functional>
#include <utility>
#include <ctime>
#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <tuple>

#include <new>
#include <memory>
#include <scoped_allocator>

#include <climits>
#include <cfloat>
#include <cstdint>
//#include <cinttypes> -- missing types from C header (for some reason)
#include <limits>

//#include <exception> -- weird issue with compiler built-in stuff
#include <stdexcept>
#include <cassert>
#include <system_error>
#include <cerrno>

#include <cctype>
#include <cwctype>
#include <cstring>
#include <cwchar>
//#include <cuchar> -- not supported on CI
#include <string>

#include <array>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>

#include <algorithm>

#include <iterator>

//#include <cmath> -- non-conforming GCC extension with regards to constexpr
//#include <complex> -- weird double include issue under MSVC
#include <valarray>
#include <random>
#include <numeric>
#include <ratio>
//#include <cfenv> -- same issue with cinttypes

#include <iosfwd>
#include <ios>
#include <istream>
#include <ostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <streambuf>
#include <cstdio>

#include <locale>
//#include <clocale> -- issue on OSX

#include <regex>

//#include <atomic> -- issue on MSVC

#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
)";

    cpp_entity_index idx;
    auto             file = parse(idx, "stdlib.cpp", code);
    parse_included_files(idx, *file);
}
