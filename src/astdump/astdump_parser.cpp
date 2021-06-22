// Copyright (C) 2017-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/astdump_parser.hpp>

#include <cstring>
#include <process.hpp>
#include <simdjson.h>

#include "parse_functions.hpp"

using namespace cppast;
namespace tpl = TinyProcessLib;
namespace dom = simdjson::ondemand; // Technically lying, as it's not simdjson::dom.

//=== config ===//
namespace
{
bool is_valid_binary(const char* binary)
{
    tpl::Process process(
        std::string(binary) + " -v", "", [](const char*, std::size_t) {},
        [](const char*, std::size_t) {});
    return process.get_exit_status() == 0;
}

const char* find_clang_binary()
{
    // Search for the valid clang binary.
    // TODO: check minimal version.
    constexpr const char* paths[] = {"./clang++",  "clang++",  "./clang-9",  "clang-9",
                                     "./clang-10", "clang-10", "./clang-11", "clang-11"};
    for (auto p : paths)
        if (is_valid_binary(p))
            return p;

    throw std::invalid_argument("unable to find clang binary");
}
} // namespace

astdump_compile_config::astdump_compile_config() : astdump_compile_config(find_clang_binary()) {}

astdump_compile_config::astdump_compile_config(std::string clang_binary_path)
: compile_config({}), clang_binary_(std::move(clang_binary_path))
{
    // Set macros to detect cppast.
    define_macro("__cppast__", name());
    define_macro("__cppast_version_major__", CPPAST_VERSION_MAJOR);
    define_macro("__cppast_version_minor__", CPPAST_VERSION_MINOR);
}

std::vector<std::string> astdump_compile_config::get_ast_dump_cmd() const
{
    std::vector<std::string> result;
    result.push_back(clang_binary_);
    for (const auto& f : get_flags())
        result.push_back(f);
    result.push_back("-Xclang");
    result.push_back("-ast-dump=json");
    result.push_back("-fsyntax-only");
    return result;
}

void astdump_compile_config::do_set_flags(cpp_standard standard, compile_flags flags)
{
    switch (standard)
    {
    case cpp_standard::cpp_98:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++98");
        else
            add_flag("-std=c++98");
        break;
    case cpp_standard::cpp_03:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++03");
        else
            add_flag("-std=c++03");
        break;
    case cpp_standard::cpp_11:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++11");
        else
            add_flag("-std=c++11");
        break;
    case cpp_standard::cpp_14:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++14");
        else
            add_flag("-std=c++14");
        break;
    case cpp_standard::cpp_1z:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++1z");
        else
            add_flag("-std=c++1z");
        break;
    case cpp_standard::cpp_17:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++17");
        else
            add_flag("-std=c++17");
        break;
    case cpp_standard::cpp_2a:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++2a");
        else
            add_flag("-std=c++2a");
        break;
    case cpp_standard::cpp_20:
        if (flags & compile_flag::gnu_extensions)
            add_flag("-std=gnu++20");
        else
            add_flag("-std=c++20");
        break;
    }

    if (flags & compile_flag::ms_compatibility)
        add_flag("-fms-compatibility");

    if (flags & compile_flag::ms_extensions)
        add_flag("-fms-extensions");
}

bool astdump_compile_config::do_enable_feature(std::string name)
{
    add_flag("-f" + std::move(name));
    return true;
}

void astdump_compile_config::do_add_include_dir(std::string path)
{
    add_flag("-I" + std::move(path));
}

void astdump_compile_config::do_add_macro_definition(std::string name, std::string definition)
{
    auto str = "-D" + std::move(name);
    if (!definition.empty())
        str += "=" + definition;
    add_flag(std::move(str));
}

void astdump_compile_config::do_remove_macro_definition(std::string name)
{
    add_flag("-U" + std::move(name));
}

//=== parser ===//
namespace
{
std::string get_ast_dump(std::vector<std::string> cmd, const std::string& path)
{
    // TODO: string escaping
    std::string cmd_str;
    for (auto c : cmd)
        cmd_str += std::move(c) + ' ';
    cmd_str += path;

    std::string  result;
    tpl::Process process(
        cmd_str, "", [&result](const char* str, std::size_t size) { result.append(str, size); },
        [](const char* str, std::size_t size) {
            // TODO: use logger instead
            std::fprintf(stderr, "%.*s", int(size), str);
        });

    auto code = process.get_exit_status();
    if (code != 0)
        throw astdump_error("ast dump command '" + cmd_str + "' failed");

    result.append(simdjson::SIMDJSON_PADDING, '\0');
    return result;
}

bool entity_is_in_main_file(dom::object& entity)
{
    dom::object location = entity["loc"];
    if (location.begin() == location.end())
        // A fake declaration inserted by clang.
        return false;

    auto expansion_loc = location["expansionLoc"];
    if (expansion_loc.error() != simdjson::error_code::NO_SUCH_FIELD)
        // Declaration was generated by a macro, actual location is this sub field.
        location = expansion_loc;

    if (location["includedFrom"].error() != simdjson::error_code::NO_SUCH_FIELD)
        // A declaration from a header file.
        return false;

    return true;
}
} // namespace

std::unique_ptr<cpp_file> astdump_parser::do_parse(const cpp_entity_index& idx, std::string path,
                                                   const compile_config& c) const
try
{
    DEBUG_ASSERT(std::strcmp(c.name(), "astdump") == 0, detail::precondition_error_handler{},
                 "config has mismatched type");
    auto& config = static_cast<const astdump_compile_config&>(c);

    // TODO: make parser persistent
    simdjson::ondemand::parser parser;
    auto                       dump = get_ast_dump(config.get_ast_dump_cmd(), path);
    auto                       ast  = parser.iterate(dump);

    // TODO: use proper flag
    if (logger().is_verbose())
    {
        std::ofstream file(path + ".json");
        file << dump.c_str();
    }

    astdump_detail::parse_context context(type_safe::ref(logger()), type_safe::ref(idx), path);
    cpp_file::builder             builder(path);
    for (dom::object entity : ast["inner"])
    {
        // Extract kinds before check as that field is logically first.
        auto kind = entity["kind"].get_string().value();

        // Skip this entity if it is not from the file.
        if (!entity_is_in_main_file(entity))
            continue;

        // Build the cpp_entity and add it to the file.
        auto e = astdump_detail::parse_entity(context, kind, entity);
        if (e)
            builder.add_child(std::move(e));
    }

    if (context.error)
        set_error();

    return builder.finish(idx);
}
catch (simdjson::simdjson_error& ex)
{
    logger().log("astdump parser",
                 format_diagnostic(severity::critical, source_location::make_file(path),
                                   "unexepted JSON for AST: ", ex.what()));
    set_error();
    return nullptr;
}

