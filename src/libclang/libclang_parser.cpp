// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/libclang_parser.hpp>

#include <cstring>
#include <fstream>
#include <vector>

#include <clang-c/CXCompilationDatabase.h>
#include <process.hpp>

#include "cxtokenizer.hpp"
#include "libclang_visitor.hpp"
#include "parse_error.hpp"
#include "parse_functions.hpp"
#include "preprocessor.hpp"
#include "raii_wrapper.hpp"

using namespace cppast;
namespace tpl = TinyProcessLib;

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

bool detail::libclang_compile_config_access::write_preprocessed(
    const libclang_compile_config& config)
{
    return config.write_preprocessed_;
}

bool detail::libclang_compile_config_access::fast_preprocessing(
    const libclang_compile_config& config)
{
    return config.fast_preprocessing_;
}

bool detail::libclang_compile_config_access::remove_comments_in_macro(
    const libclang_compile_config& config)
{
    return config.remove_comments_in_macro_;
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

bool libclang_compilation_database::has_config(const char* file_name) const
{
    auto cxcommands = clang_CompilationDatabase_getCompileCommands(database_, file_name);
    if (!cxcommands)
        return false;
    clang_CompileCommands_dispose(cxcommands);
    return true;
}

namespace
{} // namespace

libclang_compile_config::libclang_compile_config()
: compile_config({}), write_preprocessed_(false), fast_preprocessing_(false),
  remove_comments_in_macro_(false)
{
    // set given clang binary
    set_clang_binary(CPPAST_CLANG_BINARY);

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

bool has_drive_prefix(const std::string& file)
{
    return file.size() > 2 && file[1] == ':';
}

bool is_absolute(const std::string& file)
{
    return !file.empty() && (has_drive_prefix(file) || file.front() == '/' || file.front() == '\\');
}

std::string get_full_path(const detail::cxstring& dir, const std::string& file)
{
    if (is_absolute(file))
        // absolute file
        return file;
    else if (dir[dir.length() - 1] != '/' && dir[dir.length() - 1] != '\\')
        // relative needing separator
        return dir.std_str() + '/' + file;
    else
        // relative w/o separator
        return dir.std_str() + file;
}
} // namespace

void detail::for_each_file(const libclang_compilation_database& database, void* user_data,
                           void (*callback)(void*, std::string))
{
    cxcompile_commands commands(
        clang_CompilationDatabase_getAllCompileCommands(database.database_));
    auto no = clang_CompileCommands_getSize(commands.get());
    for (auto i = 0u; i != no; ++i)
    {
        auto cmd = clang_CompileCommands_getCommand(commands.get(), i);

        auto dir = cxstring(clang_CompileCommand_getDirectory(cmd));
        callback(user_data,
                 get_full_path(dir, cxstring(clang_CompileCommand_getFilename(cmd)).std_str()));
    }
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
} // namespace

libclang_compile_config::libclang_compile_config(const libclang_compilation_database& database,
                                                 const std::string&                   file)
: libclang_compile_config()
{
    auto cxcommands
        = clang_CompilationDatabase_getCompileCommands(database.database_, file.c_str());
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
                add_flag(std::move(flag) + get_full_path(dir, args));
            else if (flag == "-isystem")
                add_flag(std::move(flag) + get_full_path(dir, args));
            else if (flag == "-D" || flag == "-U")
            {
                // preprocessor options
                for (auto c : args)
                    if (c == '"')
                        flag += "\\\"";
                    else
                        flag += c;
                add_flag(std::move(flag));
            }
            else if (flag == "-std")
                // standard
                add_flag(std::move(flag) + "=" + std::move(args));
            else if (flag == "-f")
                // other options
                add_flag(std::move(flag) + std::move(args));
        });
    }
}

namespace
{
bool is_valid_binary(const std::string& binary)
{
    tpl::Process process(
        binary + " -v", "", [](const char*, std::size_t) {}, [](const char*, std::size_t) {});
    return process.get_exit_status() == 0;
}

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) && !defined(__CYGWIN__)
#    define CPPAST_DETAIL_WINDOWS 1
#else
#    define CPPAST_DETAIL_WINDOWS 0
#endif

void add_default_include_dirs(libclang_compile_config& config)
{
    std::string  verbose_output;
    tpl::Process process(
        detail::libclang_compile_config_access::clang_binary(config) + " -x c++ -v -", "",
        [](const char*, std::size_t) {},
        [&](const char* str, std::size_t n) { verbose_output.append(str, n); }, true);
    process.write("", 1);
    process.close_stdin();
    process.get_exit_status();

    auto pos = verbose_output.find("#include <...>");
    DEBUG_ASSERT(pos != std::string::npos, detail::assert_handler{});
    while (verbose_output[pos] != '\n')
        ++pos;
    ++pos;

    // now every line is an include path, starting with a space
    while (verbose_output[pos] == ' ')
    {
        auto start = pos + 1;
        while (verbose_output[pos] != '\r' && verbose_output[pos] != '\n')
            ++pos;
        auto end = pos;
        ++pos;

        auto        line = verbose_output.substr(start, end - start);
        std::string path;
        for (auto c : line)
        {
            if (c == ' ')
            { // escape spaces
#if CPPAST_DETAIL_WINDOWS
                path += "^ ";
#else
                path += "\\ ";
#endif
            }
            // clang under MacOS adds comments to paths using '(' at the end, so they have to be
            // ignored however, Windows uses '(' in paths, so they don't have to be ignored
#if !CPPAST_DETAIL_WINDOWS
            else if (c == '(')
                break;
#endif
            else
                path += c;
        }

        config.add_include_dir(std::move(path));
    }
}
} // namespace

bool libclang_compile_config::set_clang_binary(std::string binary)
{
    if (is_valid_binary(binary))
    {
        clang_binary_ = binary;
        add_default_include_dirs(*this);
        return true;
    }
    else
    {
        // first search in current directory, then in PATH
        static const char* paths[]
            = {"./clang++",   "clang++",       "./clang++-4.0", "clang++-4.0", "./clang++-5.0",
               "clang++-5.0", "./clang++-6.0", "clang++-6.0",   "./clang-7",   "clang-7",
               "./clang-8",   "clang-8",       "./clang-9",     "clang-9",     "./clang-10",
               "clang-10",    "./clang-11",    "clang-11"};
        for (auto& p : paths)
            if (is_valid_binary(p))
            {
                clang_binary_ = p;
                add_default_include_dirs(*this);
                return false;
            }

        throw std::invalid_argument("unable to find clang binary '" + binary + "'");
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
    case cpp_standard::cpp_17:
        if (libclang_parser::libclang_minor_version() >= 43)
        { // Corresponds to Clang version 5
            if (flags & compile_flag::gnu_extensions)
                add_flag("-std=gnu++17");
            else
                add_flag("-std=c++17");
            break;
        }
        else
            throw std::invalid_argument("c++17 is not yet supported for current version of clang");
    case cpp_standard::cpp_2a:
        if (libclang_parser::libclang_minor_version() >= 59)
        { // Corresponds to Clang version 9
            if (flags & compile_flag::gnu_extensions)
                add_flag("-std=gnu++2a");
            else
                add_flag("-std=c++2a");
            break;
        }
        else
            throw std::invalid_argument("c++2a is not yet supported for current version of clang");
    case cpp_standard::cpp_20:
        if (libclang_parser::libclang_minor_version() >= 60)
        { // Corresponds to Clang version 10
            if (flags & compile_flag::gnu_extensions)
                add_flag("-std=gnu++20");
            else
                add_flag("-std=c++20");
            break;
        }
        else
            throw std::invalid_argument("c++20 is not yet supported for current version of clang");
    }

    if (flags & compile_flag::ms_compatibility)
    {
        add_flag("-fms-compatibility");
        // see https://github.com/foonathan/cppast/issues/46
        define_macro("_DEBUG_FUNCTIONAL_MACHINERY", "");
    }

    if (flags & compile_flag::ms_extensions)
        add_flag("-fms-extensions");
}

bool libclang_compile_config::do_enable_feature(std::string name)
{
    add_flag("-f" + std::move(name));
    return true;
}

void libclang_compile_config::do_add_include_dir(std::string path)
{
    add_flag("-I" + std::move(path));
}

void libclang_compile_config::do_add_macro_definition(std::string name, std::string definition)
{
    auto str = "-D" + std::move(name);
    if (!definition.empty())
        str += "=" + definition;
    add_flag(std::move(str));
}

void libclang_compile_config::do_remove_macro_definition(std::string name)
{
    add_flag("-U" + std::move(name));
}

type_safe::optional<libclang_compile_config> cppast::find_config_for(
    const libclang_compilation_database& database, std::string file_name)
{
    if (database.has_config(file_name))
        return libclang_compile_config(database, std::move(file_name));

    auto dot = file_name.rfind('.');
    if (dot != std::string::npos)
        file_name.erase(dot);

    if (database.has_config(file_name))
        return libclang_compile_config(database, std::move(file_name));
    static const char* extensions[]
        = {".h", ".hpp", ".cpp", ".h++", ".c++", ".hxx", ".cxx", ".hh", ".cc", ".H", ".C"};
    for (auto ext : extensions)
    {
        auto name = file_name + ext;
        if (database.has_config(name))
            return libclang_compile_config(database, std::move(name));
    }

    return type_safe::nullopt;
}

int cppast::libclang_parser::libclang_minor_version()
{
    return CINDEX_VERSION_MINOR;
}

struct libclang_parser::impl
{
    detail::cxindex index;

    impl() : index(clang_createIndex(0, 0)) // no diagnostic, other one is irrelevant
    {}
};

libclang_parser::libclang_parser() : libclang_parser(default_logger()) {}

libclang_parser::libclang_parser(type_safe::object_ref<const diagnostic_logger> logger)
: parser(logger), pimpl_(new impl)
{}

libclang_parser::~libclang_parser() noexcept {}

namespace
{
std::vector<const char*> get_arguments(const libclang_compile_config& config)
{
    std::vector<const char*> args
        = {"-x", "c++", "-I."}; // force C++ and enable current directory for include search
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

void print_diagnostics(const diagnostic_logger& logger, const CXTranslationUnit& tu)
{
    auto no = clang_getNumDiagnostics(tu);
    for (auto i = 0u; i != no; ++i)
    {
        auto diag = clang_getDiagnostic(tu, i);
        auto sev  = get_severity(diag);
        if (sev)
        {
            auto     diag_loc = clang_getDiagnosticLocation(diag);
            CXString diag_file;
            unsigned line;
            clang_getPresumedLocation(diag_loc, &diag_file, &line, nullptr);

            auto loc  = source_location::make_file(detail::cxstring(diag_file).c_str(), line);
            auto text = detail::cxstring(clang_getDiagnosticSpelling(diag));
            if (text != "too many errors emitted, stopping now")
                logger.log("libclang", diagnostic{text.c_str(), loc, sev.value()});
        }
    }
}

detail::cxtranslation_unit get_cxunit(const diagnostic_logger& logger, const detail::cxindex& idx,
                                      const libclang_compile_config& config, const char* path,
                                      const std::string& source)
{
    CXUnsavedFile file{path, source.c_str(), static_cast<unsigned long>(source.length())};

    auto args = get_arguments(config);

    CXTranslationUnit tu;
    auto              flags = CXTranslationUnit_Incomplete | CXTranslationUnit_KeepGoing
                 | CXTranslationUnit_DetailedPreprocessingRecord;

    auto error
        = clang_parseTranslationUnit2(idx.get(), path, // index and path
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
    print_diagnostics(logger, tu);

    return detail::cxtranslation_unit(tu);
}

unsigned get_line_no(const CXCursor& cursor)
{
    auto loc = clang_getCursorLocation(cursor);

    unsigned line;
    clang_getPresumedLocation(loc, nullptr, &line, nullptr);
    return line;
}
} // namespace
std::unique_ptr<cpp_file> libclang_parser::do_parse(const cpp_entity_index& idx, std::string path,
                                                    const compile_config& c) const
try
{
    DEBUG_ASSERT(std::strcmp(c.name(), "libclang") == 0, detail::precondition_error_handler{},
                 "config has mismatched type");
    auto& config = static_cast<const libclang_compile_config&>(c);

    // preprocess
    auto preprocessed = detail::preprocess(config, path.c_str(), logger());
    if (detail::libclang_compile_config_access::write_preprocessed(config))
    {
        std::ofstream file(path + ".pp");
        file << preprocessed.source;
    }

    // parse
    auto tu   = get_cxunit(logger(), pimpl_->index, config, path.c_str(), preprocessed.source);
    auto file = clang_getFile(tu.get(), path.c_str());

    cpp_file::builder builder(detail::cxstring(clang_getFileName(file)).std_str());
    auto              macro_iter   = preprocessed.macros.begin();
    auto              include_iter = preprocessed.includes.begin();

    // convert entity hierarchies
    detail::parse_context context{tu.get(),
                                  file,
                                  type_safe::ref(logger()),
                                  type_safe::ref(idx),
                                  detail::comment_context(preprocessed.comments),
                                  false};
    detail::visit_tu(tu, path.c_str(), [&](const CXCursor& cur) {
        if (clang_getCursorKind(cur) == CXCursor_InclusionDirective)
        {
            if (!preprocessed.includes.empty())
            {
                DEBUG_ASSERT(include_iter != preprocessed.includes.end()
                                 && get_line_no(cur) >= include_iter->line,
                             detail::assert_handler{});

                auto full_path = include_iter->full_path.empty() ? include_iter->file_name
                                                                 : include_iter->full_path;

                // if we got an absolute file path for the current file,
                // also use an absolute file path for the id
                // otherwise just use the file name as written in the source file
                // note: this is a hack around lack of `fs::canonical()`
                cpp_entity_id id("");
                if (is_absolute(builder.get().name()))
                    id = cpp_entity_id(full_path.c_str());
                else
                    id = cpp_entity_id(include_iter->file_name.c_str());

                auto include
                    = cpp_include_directive::build(cpp_file_ref(id,
                                                                std::move(include_iter->file_name)),
                                                   include_iter->kind, std::move(full_path));
                context.comments.match(*include, include_iter->line,
                                       false); // must not skip comments,
                                               // includes are not reported in order
                builder.add_child(std::move(include));

                ++include_iter;
            }
        }
        else if (clang_getCursorKind(cur) != CXCursor_MacroDefinition
                 && clang_getCursorKind(cur) != CXCursor_MacroExpansion)
        {
            // add macro if needed
            for (auto line = get_line_no(cur);
                 macro_iter != preprocessed.macros.end() && macro_iter->line <= line; ++macro_iter)
                builder.add_child(std::move(macro_iter->macro));

            auto entity = detail::parse_entity(context, &builder.get(), cur);
            if (entity)
                builder.add_child(std::move(entity));
        }
    });

    for (; macro_iter != preprocessed.macros.end(); ++macro_iter)
        builder.add_child(std::move(macro_iter->macro));

    for (auto& cur : preprocessed.comments)
    {
        if (!cur.comment.empty())
            builder.add_unmatched_comment(cpp_doc_comment(std::move(cur.comment), cur.line));
    }

    if (context.error)
        set_error();

    return builder.finish(idx);
}
catch (detail::parse_error& ex)
{
    logger().log("libclang parser", ex.get_diagnostic(path));
    set_error();
    return nullptr;
}
