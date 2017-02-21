// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/libclang_parser.hpp>

#include <cstring>
#include <vector>

#include "libclang_visitor.hpp"
#include "raii_wrapper.hpp"
#include "parse_error.hpp"
#include "preprocessor.hpp"
#include "tokenizer.hpp"

using namespace cppast;

const std::string& detail::libclang_compile_config_access::clang_binary(
    const libclang_compile_config& config)
{
    return config.clang_binary_;
}

const std::vector<std::string>& detail::libclang_compile_config_access::flags(
    const libclang_compile_config& config)
{
    return config.get_flags();
}

libclang_compile_config::libclang_compile_config() : compile_config({})
{
    set_clang_binary("clang++");
    add_include_dir(LIBCLANG_SYSTEM_INCLUDE_DIR);
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

libclang_parser::libclang_parser(type_safe::object_ref<const diagnostic_logger> logger)
: parser(logger), pimpl_(new impl)
{
}

libclang_parser::~libclang_parser() noexcept
{
}

namespace
{
    std::vector<const char*> get_arguments(const libclang_compile_config& config)
    {
        std::vector<const char*> args = {"-x", "c++"}; // force C++
        for (auto& flag : detail::libclang_compile_config_access::flags(config))
            args.push_back(flag.c_str());
        return args;
    }

    detail::cxtranslation_unit get_cxunit(const detail::cxindex&         idx,
                                          const libclang_compile_config& config, const char* path,
                                          const std::string& source)
    {
        CXUnsavedFile file;
        file.Filename = path;
        file.Contents = source.c_str();
        file.Length   = source.length();

        auto args = get_arguments(config);

        CXTranslationUnit tu;
        auto              error =
            clang_parseTranslationUnit2(idx.get(), path, // index and path
                                        args.data(),
                                        static_cast<int>(args.size()), // arguments (ptr + size)
                                        &file, 1,                      // unsaved files (ptr + size)
                                        CXTranslationUnit_Incomplete
                                            | CXTranslationUnit_KeepGoing, // flags
                                        &tu);
        if (error != CXError_Success)
        {
            switch (error)
            {
            case CXError_Success:
                DEBUG_UNREACHABLE(detail::assert_handler{});
                break;

            case CXError_Failure:
                throw libclang_error("clang_parseTranslationUnit: generic error");
            case CXError_Crashed:
                throw libclang_error("clang_parseTranslationUnit: libclang crashed :(");
            case CXError_InvalidArguments:
                throw libclang_error("clang_parseTranslationUnit: you shouldn't see this message");
            case CXError_ASTReadError:
                throw libclang_error("clang_parseTranslationUnit: AST deserialization error");
            }
        }

        return tu;
    }
}

std::unique_ptr<cpp_file> libclang_parser::do_parse(const cpp_entity_index& idx, std::string path,
                                                    const compile_config& c) const try
{
    DEBUG_ASSERT(std::strcmp(c.name(), "libclang") == 0, detail::precondition_error_handler{},
                 "config has mismatched type");
    auto& config = static_cast<const libclang_compile_config&>(c);

    // preprocess + parse
    auto preprocessed = detail::preprocess(config, path.c_str(), logger());
    auto tu           = get_cxunit(pimpl_->index, config, path.c_str(), preprocessed.source);
    auto file         = clang_getFile(tu.get(), path.c_str());

    // convert entity hierachies
    cpp_file::builder builder(path);

    // add all preprocessor entities up-front
    // TODO: add them in the correct place
    for (auto& e : preprocessed.entities)
        builder.add_child(std::move(e.entity));

    detail::visit_tu(tu, path.c_str(), [&](const CXCursor&) {});

    return builder.finish(idx);
}
catch (detail::parse_error& ex)
{
    logger().log("libclang parser", ex.get_diagnostic());
    return cpp_file::builder(path).finish(idx);
}
