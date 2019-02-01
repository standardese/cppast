// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "test_parser.hpp"

#include <cppast/cpp_preprocessor.hpp>

using namespace cppast;

TEST_CASE("stdlib", "[!hide][integration]")
{
    auto code = R"(
// list of headers from: http://en.cppreference.com/w/cpp/header

//#include <cstdlib> -- problem with compiler built-in stuff on OSX
#include <csignal>
//#include <csetjmp> -- same as above
#include <bitset>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <ctime>
#include <functional>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

#include <memory>
//#include <new> -- something weird going on here
#include <scoped_allocator>

#include <cfloat>
#include <climits>
#include <cstdint>
//#include <cinttypes> -- missing types from C header (for some reason)
#include <limits>

//#include <exception> -- weird issue with compiler built-in stuff
#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <system_error>

#include <cctype>
#include <cstring>
#include <cwchar>
#include <cwctype>
//#include <cuchar> -- not supported on CI
#include <string>

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <algorithm>

#include <iterator>

//#include <cmath> -- non-conforming GCC extension with regards to constexpr
//#include <complex> -- weird double include issue under MSVC
#include <numeric>
#include <random>
#include <ratio>
#include <valarray>
//#include <cfenv> -- same issue with cinttypes

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>

#include <locale>
//#include <clocale> -- issue on OSX

#include <regex>

//#include <atomic> -- issue on MSVC

#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>
)";
    write_file("stdlib.cpp", code);

    cpp_entity_index                    idx;
    simple_file_parser<libclang_parser> parser(type_safe::ref(idx), default_logger());

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);
    auto file = parser.parse("stdlib.cpp", config);
    REQUIRE(!parser.error());
    REQUIRE(file);

    REQUIRE(resolve_includes(parser, file.value(), config) == 61);
    REQUIRE(!parser.error());
}

TEST_CASE("cppast", "[!hide][integration]")
{
    const char* files[] = {
#include <cppast_files.hpp>
    };

    cpp_entity_index                    idx;
    simple_file_parser<libclang_parser> parser(type_safe::ref(idx), default_logger());

    libclang_compilation_database database(CPPAST_COMPILE_COMMANDS);
    libclang_compile_config       config(database, CPPAST_INTEGRATION_FILE);
    config.fast_preprocessing(true);
    parse_files(parser, files, config);

    REQUIRE(!parser.error());
}
