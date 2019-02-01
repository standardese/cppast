// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
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
    // skip until including __cppast_version__minor__, those are the default options
    auto in_default = true;
    for (auto& config_flag : config_flags)
    {
        if (config_flag == "-D__cppast_version_minor__=" CPPAST_VERSION_MINOR)
            in_default = false;
        else if (!in_default)
            result += config_flag + ' ';
    }
    REQUIRE(!result.empty());
    result.pop_back();
    REQUIRE(result == flags);
}

TEST_CASE("libclang_compile_config")
{
// only test database parser
#ifdef _WIN32
    auto json = R"([
{
    "directory": "C:/foo",
    "command": "/usr/bin/clang++ -Irelative -IC:/absolute -DA=FOO -DB(X)=X -c -o a.o a.cpp",
    "file": "a.cpp"
},
{
    "directory": "C:/bar/",
    "command": "/usr/bin/clang++ -Irelative -DA=FOO -c -o b.o b.cpp",
    "file": "C:/b.cpp",
},
{
    "directory": "C:/bar/",
    "command": "/usr/bin/clang++ -IC:/absolute -DB(X)=X -c -o b.o b.cpp",
    "file": "C:/b.cpp",
},
{
    "directory": "",
    "command": "/usr/bin/clang++ -std=c++14 -fms-extensions -fms-compatibility -fno-strict-aliasing -c -o c.o c.cpp",
    "file": "C:/c.cpp",
}
])";

#    define CPPAST_DETAIL_DRIVE "C:"

#else
    auto json = R"([
{
    "directory": "/foo",
    "command": "/usr/bin/clang++ -Irelative -I/absolute -DA=FOO -DB(X)=X -c -o a.o a.cpp",
    "file": "a.cpp"
},
{
    "directory": "/bar/",
    "command": "/usr/bin/clang++ -Irelative -DA=FOO -c -o b.o b.cpp",
    "file": "/b.cpp",
},
{
    "directory": "/bar/",
    "command": "/usr/bin/clang++ -I/absolute -DB(X)=X -c -o b.o b.cpp",
    "file": "/b.cpp",
},
{
    "directory": "",
    "command": "/usr/bin/clang++ -std=c++14 -fms-extensions -fms-compatibility -fno-strict-aliasing -c -o c.o c.cpp",
    "file": "/c.cpp",
}
])";

#    define CPPAST_DETAIL_DRIVE

#endif

    auto database = get_database(json);

    libclang_compile_config a(database, CPPAST_DETAIL_DRIVE "/foo/a.cpp");
    require_flags(a, "-I" CPPAST_DETAIL_DRIVE "/foo/relative -I" CPPAST_DETAIL_DRIVE
                     "/absolute -DA=FOO -DB(X)=X");

    libclang_compile_config b(database, CPPAST_DETAIL_DRIVE "/b.cpp");
    require_flags(b, "-I" CPPAST_DETAIL_DRIVE "/bar/relative -DA=FOO -I" CPPAST_DETAIL_DRIVE
                     "/absolute -DB(X)=X");

    libclang_compile_config c(database, CPPAST_DETAIL_DRIVE "/c.cpp");
    require_flags(c, "-std=c++14 -fms-extensions -fms-compatibility -fno-strict-aliasing");
}
