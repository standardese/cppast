// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/libclang_parser.hpp>

#include <cstring>
#include <vector>

#include "libclang_visitor.hpp"
#include "raii_wrapper.hpp"
#include "parse_error.hpp"
#include "parse_functions.hpp"
#include "preprocessor.hpp"
#include "tokenizer.hpp"

using namespace cppast;

const std::string& detail::libclang_compile_config_access::clang_binary(
    const libclang_compile_config& config)
{
    return config.clang_binary_;
}

int detail::libclang_compile_config_access::clang_version(const libclang_compile_config& config)
{
    return config.clang_version_;
}

const std::vector<std::string>& detail::libclang_compile_config_access::flags(
    const libclang_compile_config& config)
{
    return config.get_flags();
}

namespace
{
    int parse_number(const char*& str)
    {
        auto result = 0;
        for (; *str && *str != '.'; ++str)
        {
            result *= 10;
            result += int(*str - '0');
        }
        return result;
    }
}

libclang_compile_config::libclang_compile_config() : compile_config({})
{
    // set given clang binary
    auto ptr   = CPPAST_CLANG_VERSION_STRING;
    auto major = parse_number(ptr);
    auto minor = parse_number(ptr);
    auto patch = parse_number(ptr);
    set_clang_binary(CPPAST_CLANG_BINARY, major, minor, patch);

    // set system include dir
    add_include_dir(CPPAST_LIBCLANG_SYSTEM_INCLUDE_DIR);

    // set macros to detect cppast
    define_macro("__cppast__", "libclang");
    define_macro("__cppast_version_major__", CPPAST_VERSION_MAJOR);
    define_macro("__cppast_version_minor__", CPPAST_VERSION_MINOR);
}

void libclang_compile_config::do_set_flags(cpp_standard standard, compile_flags flags)
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

    impl() : index(clang_createIndex(0, 0)) // no diagnostic, other one is irrelevant
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
        std::vector<const char*> args =
            {"-x", "c++", "-I."}; // force C++ and enable current directory for include search
        for (auto& flag : detail::libclang_compile_config_access::flags(config))
            args.push_back(flag.c_str());
        return args;
    }

    type_safe::optional<severity> get_severity(const CXDiagnostic& diag)
    {
        switch (clang_getDiagnosticSeverity(diag))
        {
        case CXDiagnostic_Ignored:
        case CXDiagnostic_Note:
        case CXDiagnostic_Warning:
            // ignore those diagnostics
            return type_safe::nullopt;

        case CXDiagnostic_Error:
            return severity::error;
        case CXDiagnostic_Fatal:
            return severity::critical;
        }

        DEBUG_UNREACHABLE(detail::assert_handler{});
        return type_safe::nullopt;
    }

    void print_diagnostics(const diagnostic_logger& logger, const char* path,
                           const CXTranslationUnit& tu)
    {
        auto no = clang_getNumDiagnostics(tu);
        for (auto i = 0u; i != no; ++i)
        {
            auto diag = clang_getDiagnostic(tu, i);
            auto sev  = get_severity(diag);
            if (sev)
            {
                auto loc  = source_location::make_file(path); // line number won't help
                auto text = detail::cxstring(clang_getDiagnosticSpelling(diag));

                logger.log("libclang", diagnostic{text.c_str(), loc, sev.value()});
            }
        }
    }

    detail::cxtranslation_unit get_cxunit(const diagnostic_logger&       logger,
                                          const detail::cxindex&         idx,
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
        print_diagnostics(logger, path, tu);

        return detail::cxtranslation_unit(tu);
    }

    unsigned get_line_no(const CXCursor& cursor)
    {
        auto loc = clang_getCursorLocation(cursor);

        unsigned line;
        clang_getPresumedLocation(loc, nullptr, &line, nullptr);
        return line;
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
    auto tu   = get_cxunit(logger(), pimpl_->index, config, path.c_str(), preprocessed.source);
    auto file = clang_getFile(tu.get(), path.c_str());

    cpp_file::builder builder(path);
    auto              preprocessed_iter = preprocessed.entities.begin();

    // convert entity hierachies
    detail::parse_context context{tu.get(), file, type_safe::ref(logger()), type_safe::ref(idx),
                                  detail::comment_context(preprocessed.comments)};
    detail::visit_tu(tu, path.c_str(), [&](const CXCursor& cur) {
        // add macro if needed
        for (auto line = get_line_no(cur);
             preprocessed_iter != preprocessed.entities.end() && preprocessed_iter->line <= line;
             ++preprocessed_iter)
            builder.add_child(std::move(preprocessed_iter->entity));

        auto entity = detail::parse_entity(context, cur);
        if (entity)
            builder.add_child(std::move(entity));
    });

    for (; preprocessed_iter != preprocessed.entities.end(); ++preprocessed_iter)
        builder.add_child(std::move(preprocessed_iter->entity));

    for (auto& c : preprocessed.comments)
    {
        if (!c.comment.empty())
            builder.add_unmatched_comment(std::move(c.comment));
    }

    return builder.finish(idx);
}
catch (detail::parse_error& ex)
{
    logger().log("libclang parser", ex.get_diagnostic());
    return cpp_file::builder(path).finish(idx);
}
