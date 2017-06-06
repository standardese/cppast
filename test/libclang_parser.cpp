// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <catch.hpp>

#include <cppast/libclang_parser.hpp>

#include <fstream>

using namespace cppast;

libclang_compilation_database get_database(const char* json)
{
    std::ofstream file("compile_commands.json");
    file << json;
    file.close();

    return libclang_compilation_database(".");
}

void require_flags(const libclang_compile_config& config, const char* flags)
{
    std::string result;
    auto        config_flags = detail::libclang_compile_config_access::flags(config);
    // skip first 4, those are the default options
    for (auto iter = config_flags.begin() + 4; iter != config_flags.end(); ++iter)
        result += *iter + ' ';
    result.pop_back();
    REQUIRE(result == flags);
}

TEST_CASE("libclang_compile_config")
{
    // only test database parser
    auto json = R"([
{
    "directory": "/foo",
    "command": "/usr/bin/clang++ -Irelative -I/absolute -DA=FOO -DB(X)=X -c -o a.o a.cpp",
    "file": "a.cpp"
},
{
    "directory": "/bar/",
    "command": "/usr/bin/clang++ -Irelative -DA=FOO -c -o b.o b.cpp",
    "command": "/usr/bin/clang++ -I/absolute -DB(X)=X -c -o b.o b.cpp",
    "file": "/b.cpp",
},
{
    "directory": "/bar/",
    "command": "/usr/bin/clang++ -I/absolute -DB(X)=X -c -o b.o b.cpp",
    "file": "/b.cpp",
},
{
    "directory": "",
    "command": "/usr/bin/clang++ -std=c++14 -fms-extensions -fms-compatibility -c -o c.o c.cpp",
    "file": "/c.cpp",
}
])";

    auto database = get_database(json);

    libclang_compile_config a(database, "/foo/a.cpp");
    require_flags(a, "-I/foo/relative -I/absolute -DA=FOO -DB(X)=X");

    libclang_compile_config b(database, "/b.cpp");
    require_flags(b, "-I/bar/relative -DA=FOO -I/absolute -DB(X)=X");

    libclang_compile_config c(database, "/c.cpp");
    require_flags(c, "-std=c++14 -fms-extensions -fms-compatibility");
}
