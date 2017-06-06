// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/libclang_parser.hpp>

#include <cstring>
#include <vector>

#include <clang-c/CXCompilationDatabase.h>

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

libclang_compilation_database::libclang_compilation_database(const std::string& build_directory)
{
    static_assert(std::is_same<database, CXCompilationDatabase>::value, "forgot to update type");

    auto error = CXCompilationDatabase_NoError;
    database_  = clang_CompilationDatabase_fromDirectory(build_directory.c_str(), &error);
    if (error != CXCompilationDatabase_NoError)
        throw libclang_error("unable to load compilation database");
}

libclang_compilation_database::~libclang_compilation_database()
{
    if (database_)
        clang_CompilationDatabase_dispose(database_);
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

namespace
{
    struct cxcompile_commands_deleter
    {
        void operator()(CXCompileCommands cmds)
        {
            clang_CompileCommands_dispose(cmds);
        }
    };

    using cxcompile_commands = detail::raii_wrapper<CXCompileCommands, cxcompile_commands_deleter>;
}

namespace
{
    bool is_flag(const detail::cxstring& str)
    {
        return str.length() > 1u && str[0] == '-';
    }

    const char* find_flag_arg_sep(const std::string& last_flag)
    {
        if (last_flag[1] == 'D')
            // no  separator, equal is part of the arg
            return nullptr;
        return std::strchr(last_flag.c_str(), '=');
    }

    template <typename Func>
    void parse_flags(CXCompileCommand cmd, Func callback)
    {
        auto        no_args = clang_CompileCommand_getNumArgs(cmd);
        std::string last_flag;
        for (auto i = 1u /* 0 is compiler executable */; i != no_args; ++i)
        {
            detail::cxstring str(clang_CompileCommand_getArg(cmd, i));
            if (is_flag(str))
            {
                if (!last_flag.empty())
                {
                    // process last flag
                    std::string args;
                    if (auto ptr = find_flag_arg_sep(last_flag))
                    {
                        auto pos = std::size_t(ptr - last_flag.c_str());
                        ++ptr;
                        while (*ptr)
                            args += *ptr++;
                        last_flag.erase(pos);
                    }
                    else if (last_flag.size() > 2u)
                    {
                        // assume two character flag
                        args = last_flag.substr(2u);
                        last_flag.erase(2u);
                    }

                    callback(std::move(last_flag), std::move(args));
                }

                last_flag = str.std_str();
            }
            else if (!last_flag.empty())
            {
                // we have flags + args
                callback(std::move(last_flag), str.std_str());
                last_flag.clear();
            }
            // else skip argument
        }
    }
}

libclang_compile_config::libclang_compile_config(const libclang_compilation_database& database,
                                                 const std::string&                   file)
: libclang_compile_config()
{
    auto cxcommands =
        clang_CompilationDatabase_getCompileCommands(database.database_, file.c_str());
    if (cxcommands == nullptr)
        throw libclang_error(detail::format("no compile commands specified for file '", file, "'"));
    cxcompile_commands commands(cxcommands);

    auto size = clang_CompileCommands_getSize(commands.get());
    for (auto i = 0u; i != size; ++i)
    {
        auto cmd = clang_CompileCommands_getCommand(commands.get(), i);
        auto dir = detail::cxstring(clang_CompileCommand_getDirectory(cmd));
        parse_flags(cmd, [&](std::string flag, std::string args) {
            if (flag == "-I")
            {
                if (args.front() == '/' || args.front() == '\\')
                {
                    add_flag(std::move(flag) + std::move(args));
                }
                else
                {
                    // path relative to the directory
                    if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
                        add_flag(std::move(flag) + dir.std_str() + '/' + std::move(args));
                    else
                        add_flag(std::move(flag) + dir.std_str() + std::move(args));
                }
            }
            else if (flag == "-D" || flag == "-U")
                // preprocessor options
                this->add_flag(std::move(flag) + std::move(args));
            else if (flag == "-std")
                // standard
                this->add_flag(std::move(flag) + "=" + std::move(args));
            else if (flag == "-f" && (args == "ms-compatibility" || args == "ms-extensions"))
                // other options
                this->add_flag(std::move(flag) + std::move(args));
        });
    }
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
        auto              flags = CXTranslationUnit_Incomplete | CXTranslationUnit_KeepGoing;
        if (detail::libclang_compile_config_access::clang_version(config) >= 40000)
            flags |= CXTranslationUnit_DetailedPreprocessingRecord;

        auto error =
            clang_parseTranslationUnit2(idx.get(), path, // index and path
                                        args.data(),
                                        static_cast<int>(args.size()), // arguments (ptr + size)
                                        &file, 1,                      // unsaved files (ptr + size)
                                        unsigned(flags), &tu);
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
    auto              macro_iter   = preprocessed.macros.begin();
    auto              include_iter = preprocessed.includes.begin();

    // convert entity hierachies
    detail::parse_context context{tu.get(), file, type_safe::ref(logger()), type_safe::ref(idx),
                                  detail::comment_context(preprocessed.comments)};
    detail::visit_tu(tu, path.c_str(), [&](const CXCursor& cur) {
        if (clang_getCursorKind(cur) == CXCursor_InclusionDirective)
        {
            DEBUG_ASSERT(include_iter != preprocessed.includes.end()
                             && get_line_no(cur) >= include_iter->line,
                         detail::assert_handler{});

            auto include =
                cpp_include_directive::build(std::move(include_iter->file), include_iter->kind,
                                             detail::get_cursor_name(cur).c_str());
            context.comments.match(*include, include_iter->line);
            builder.add_child(std::move(include));

            ++include_iter;
        }
        else if (clang_getCursorKind(cur) != CXCursor_MacroDefinition)
        {
            // add macro if needed
            for (auto line = get_line_no(cur);
                 macro_iter != preprocessed.macros.end() && macro_iter->line <= line; ++macro_iter)
                builder.add_child(std::move(macro_iter->macro));

            auto entity = detail::parse_entity(context, cur);
            if (entity)
                builder.add_child(std::move(entity));
        }
    });

    for (; macro_iter != preprocessed.macros.end(); ++macro_iter)
        builder.add_child(std::move(macro_iter->macro));

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
