// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/libclang_parser.hpp>

#include "raii_wrapper.hpp"

using namespace cppast;

libclang_compile_config::libclang_compile_config() : compile_config({})
{
}

void libclang_compile_config::do_set_flags(cpp_standard                      standard,
                                           type_safe::flag_set<compile_flag> flags)
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
    }

    if (flags & compile_flag::ms_compatibility)
        add_flag("-fms-compatibility");
    if (flags & compile_flag::ms_extensions)
        add_flag("-fms-extensions");
}

void libclang_compile_config::do_add_include_dir(std::string path)
{
    add_flag("-I" + std::move(path));
}

void libclang_compile_config::do_add_macro_definition(std::string name, std::string definition)
{
    auto str = "-D" + std::move(name);
    if (!definition.empty())
        str += "=" + std::move(definition);
    add_flag(std::move(str));
}

void libclang_compile_config::do_remove_macro_definition(std::string name)
{
    add_flag("-U" + std::move(name));
}

struct libclang_parser::impl
{
    detail::cxindex index;

    impl() : index(clang_createIndex(1, 1))
    {
    }
};

libclang_parser::libclang_parser()
{
}

libclang_parser::~libclang_parser() noexcept
{
}

std::unique_ptr<cpp_file> libclang_parser::do_parse(const cpp_entity_index& idx,
                                                    const std::string&      path,
                                                    const compile_config&   config) const
{
    return nullptr;
}
