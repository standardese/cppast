// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "preprocessor.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <unordered_map>

#include <process.hpp>

#include <cppast/diagnostic.hpp>

#include "parse_error.hpp"

using namespace cppast;
namespace tpl = TinyProcessLib;
namespace ts  = type_safe;

bool detail::pp_doc_comment::matches(const cpp_entity&, unsigned e_line)
{
    if (kind == detail::pp_doc_comment::end_of_line)
        return line == e_line;
    else
        return line + 1u == e_line;
}

namespace
{
//=== diagnostic parsing ===//
source_location parse_source_location(const char*& ptr)
{
    // format: <filename>(<line>):
    // or: <filename>:
    auto        fallback = ptr;
    std::string filename;
    while (*ptr && *ptr != ':' && *ptr != '(')
        filename.push_back(*ptr++);

    if (filename == "error" || filename == "warning" || filename == "fatal error")
    {
        ptr = fallback;
        return {};
    }

    type_safe::optional<unsigned> line;
    if (*ptr == '(')
    {
        ++ptr;

        std::string str;
        while (*ptr != ')')
            str.push_back(*ptr++);
        ++ptr;

        line = unsigned(std::stoi(str));
    }

    DEBUG_ASSERT(*ptr == ':', detail::assert_handler{});
    ++ptr;

    return {type_safe::nullopt, std::move(filename), std::move(line), type_safe::nullopt};
}

severity parse_severity(const char*& ptr)
{
    // format: <severity>:
    auto        fallback = ptr;
    std::string sev;
    while (*ptr && *ptr != ':')
        sev.push_back(*ptr++);
    ++ptr;

    if (sev == "warning")
        return severity::warning;
    else if (sev == "error")
        return severity::error;
    else if (sev == "fatal error")
        return severity::critical;
    else
        ptr = fallback;
    return severity::error;
}

// parse and log diagnostic
void log_diagnostic(const diagnostic_logger& logger, const std::string& msg)
{
    auto ptr = msg.c_str();

    auto loc = parse_source_location(ptr);
    while (*ptr == ' ')
        ++ptr;

    auto sev = parse_severity(ptr);
    while (*ptr == ' ')
        ++ptr;

    std::string message;
    while (*ptr && *ptr != '\n')
        message.push_back(*ptr++);

    logger.log("preprocessor", diagnostic{std::move(message), std::move(loc), sev});
}

// parses missing header file diagnostic and returns the file name,
// if it is a missing header file diagnostic
ts::optional<std::string> parse_missing_file(const std::string& cur_file, const std::string& msg)
{
    auto ptr = msg.c_str();

    auto loc = parse_source_location(ptr);
    if (loc.file != cur_file)
        return type_safe::nullopt;

    while (*ptr == ' ')
        ++ptr;

    parse_severity(ptr);
    while (*ptr == ' ')
        ++ptr;

    // format 'file-name' file not found
    if (*ptr != '\'')
        return ts::nullopt;
    ++ptr;

    std::string filename;
    while (*ptr != '\'')
        filename += *ptr++;
    ++ptr;

    if (std::strcmp(ptr, " file not found") == 0)
        return std::move(filename);
    else
        throw libclang_error("preprocessor: unexpected diagnostic '" + msg + "'");
}

//=== external preprocessor invocation ==//
// quote a string
std::string quote(std::string str)
{
    return '"' + std::move(str) + '"';
}

std::string diagnostics_flags()
{
    std::string flags;

    // -fno-caret-diagnostics: don't show the source extract in diagnostics
    // -fno-show-column: don't show the column number
    // -fdiagnostics-format msvc: use easier to parse MSVC format
    flags += " -fno-caret-diagnostics -fno-show-column -fdiagnostics-format=msvc";
    // -Wno-*: hide wrong warnings if header file is directly parsed/duplicate macro handling
    flags += " -Wno-macro-redefined -Wno-pragma-once-outside-header "
             "-Wno-pragma-system-header-outside-header "
             "-Wno-include-next-outside-header";

    return flags;
}

// get the command that returns all macros defined in the TU
std::string get_macro_command(const libclang_compile_config& c, const char* full_path)
{
    // -x c++: force C++ as input language
    // -I.: add current working directory to include search path
    // -E: print preprocessor output
    // -dM: print macro definitions instead of preprocessed file
    auto flags = std::string("-x c++ -I. -E -dM");
    flags += diagnostics_flags();

    std::string cmd(detail::libclang_compile_config_access::clang_binary(c) + " " + std::move(flags)
                    + " ");
    // other flags
    for (auto& flag : detail::libclang_compile_config_access::flags(c))
    {
        cmd += quote(flag);
        cmd += ' ';
    }

    return cmd + quote(full_path);
}

// get the command that preprocess a translation unit given the macros
// macro_file_path == nullptr <=> don't do fast preprocessing
std::string get_preprocess_command(const libclang_compile_config& c, const char* full_path,
                                   const char* macro_file_path)
{
    // -x c++: force C++ as input language
    // -E: print preprocessor output
    // -dD: keep macros
    auto flags = std::string("-x c++ -E -dD");

    // -CC: keep comments, even in macro
    // -C: keep comments, but not in macro
    if (!detail::libclang_compile_config_access::remove_comments_in_macro(c))
        flags += " -CC";
    else
        flags += " -C";

    if (macro_file_path)
        // -no*: disable default include search paths
        flags += " -nostdinc -nostdinc++";

    // -Xclang -dI: print include directives as well
    flags += " -Xclang -dI";

    flags += diagnostics_flags();

    if (macro_file_path)
    {
        // include file that defines all macros
        flags += " -include ";
        flags += macro_file_path;
    }

    std::string cmd(detail::libclang_compile_config_access::clang_binary(c) + " " + std::move(flags)
                    + " ");

    // other flags
    for (const auto& flag : detail::libclang_compile_config_access::flags(c))
    {
        DEBUG_ASSERT(flag.size() >= 2u && flag[0] == '-', detail::assert_handler{},
                     ("\"" + flag + "\" that's an odd flag").c_str());
        if (!macro_file_path || flag[1] != 'I')
        {
            // only add this flag if it is not an include or we're not doing fast preprocessing
            cmd += quote(flag);
            cmd += ' ';
        }
    }

    return cmd + quote(full_path);
}

std::string get_macro_file_name()
{
    static std::atomic<unsigned> counter(0u);
    return "standardese-macro-file-" + std::to_string(++counter) + ".delete-me";
}

template <std::size_t N>
void bump_until(std::istreambuf_iterator<char>& iter, const char (&str)[N])
{
    auto ptr = &str[0];
    while (ptr != &str[N - 1])
    {
        if (iter == std::istreambuf_iterator<char>{})
            // end of file
            break;
        else if (*iter != *ptr)
        {
            // try again
            ptr = &str[0];
            if (*iter == *ptr)
                ++ptr; // it was the first character again
        }
        else
            // okay, move forward
            ++ptr;

        ++iter;
    }
}

template <typename Iter>
void skip_whitespace(Iter& begin, Iter end)
{
    while (begin != end && (*begin == ' ' || *begin == '\t'))
        ++begin;
}

template <typename Iter>
std::string get_line(Iter& begin, Iter end)
{
    std::string line;
    while (begin != end && *begin != '\n')
        line += *begin++;
    ++begin; // newline
    return line;
}

type_safe::optional<std::string> get_include_guard_macro(const std::string& full_path)
{
    std::ifstream file(full_path);

    auto iter = std::istreambuf_iterator<char>(file);
    while (iter != std::istreambuf_iterator<char>{})
    {
        if (*iter == '/')
        {
            ++iter;
            if (*iter == '/')
                // C++ style comment, bump until \n
                bump_until(iter, "\n");
            else if (*iter == '*')
                // C style comment
                bump_until(iter, "*/");
        }
        else if (*iter == ' ' || *iter == '\t' || *iter == '\n')
            ++iter; // empty
        else if (*iter == '#')
        {
            // preprocessor line
            auto if_line = get_line(iter, {});
            if (if_line.compare(0, 3, "#if") != 0)
                // not something starting with #if
                break;

            skip_whitespace(iter, {});

            auto macro_line = get_line(iter, {});
            if (macro_line.compare(0, 7, "#define") != 0)
                // not a corresponding define
                break;

            auto macro_name_begin = std::next(macro_line.begin(), 7);
            // skip whitespace after define
            skip_whitespace(macro_name_begin, macro_line.end());

            auto macro_name_end = macro_name_begin;
            // skip over identifier
            while (macro_name_end != macro_line.end()
                   && (*macro_name_end == '_' || std::isalnum(*macro_name_end)))
                ++macro_name_end;

            auto trailing_ws = macro_line.rbegin();
            skip_whitespace(trailing_ws, macro_line.rend());
            if (macro_name_end != trailing_ws.base())
                // anything else after macro
                break;

            std::string macro_name(macro_name_begin, macro_name_end);
            if (if_line.find(macro_name) == std::string::npos)
                // macro name doesn't occur in if line
                break;
            else
                return macro_name;
        }
        else
            // line is neither empty, comment, nor preprocessor
            break;
    }

    // assume no include guard followed a bad line
    return type_safe::nullopt;
}

std::string write_macro_file(const libclang_compile_config& c, const std::string& full_path,
                             const diagnostic_logger& logger)
{
    std::string diagnostic;
    auto        diagnostic_logger = [&](const char* str, std::size_t n) {
        diagnostic.reserve(diagnostic.size() + n);
        for (auto end = str + n; str != end; ++str)
            if (*str == '\r')
                continue;
            else if (*str == '\n')
            {
                // consume current diagnostic
                log_diagnostic(logger, diagnostic);
                diagnostic.clear();
            }
            else
                diagnostic.push_back(*str);
    };

    auto          file = get_macro_file_name();
    std::ofstream stream(file);

    auto         cmd = get_macro_command(c, full_path.c_str());
    tpl::Process process(cmd, "",
                         [&](const char* str, std::size_t n) {
                             stream.write(str, std::streamsize(n));
                         },
                         diagnostic_logger);

    if (auto include_guard = get_include_guard_macro(full_path))
        // undefine include guard
        stream << "#undef " << include_guard.value();

    auto exit_code = process.get_exit_status();
    DEBUG_ASSERT(diagnostic.empty(), detail::assert_handler{});
    if (exit_code != 0)
        throw libclang_error("preprocessor (macro): command '" + cmd
                             + "' exited with non-zero exit code (" + std::to_string(exit_code)
                             + ")");
    return file;
}

struct clang_preprocess_result
{
    std::string              file;
    std::vector<std::string> included_files; // needed for pre-clang 4.0.0
};

clang_preprocess_result clang_preprocess_impl(const libclang_compile_config& c,
                                              const diagnostic_logger&       logger,
                                              const std::string& full_path, const char* macro_path)
{
    clang_preprocess_result result;

    std::string diagnostic;
    auto        expect_bad_exit_code = false;
    auto        diagnostic_handler   = [&](const char* str, std::size_t n) {
        diagnostic.reserve(diagnostic.size() + n);
        for (auto end = str + n; str != end; ++str)
            if (*str == '\r')
                continue;
            else if (*str == '\n')
            {
                // handle current diagnostic
                if (macro_path)
                {
                    // hide diagnostics

                    auto file = parse_missing_file(full_path, diagnostic);
                    if (file)
                        // save for clang without -dI flag
                        result.included_files.push_back(file.value());

                    expect_bad_exit_code = true;
                }
                else
                    log_diagnostic(logger, diagnostic);

                diagnostic.clear();
            }
            else
                diagnostic.push_back(*str);
    };

    auto         cmd = get_preprocess_command(c, full_path.c_str(), macro_path);
    tpl::Process process(cmd, "",
                         [&](const char* str, std::size_t n) {
                             for (auto ptr = str; ptr != str + n; ++ptr)
                                 if (*ptr == '\t')
                                     result.file += ' '; // convert to single spaces
                                 else if (*ptr != '\r')
                                     result.file += *ptr;
                         },
                         diagnostic_handler);
    // wait for process end
    auto exit_code = process.get_exit_status();
    DEBUG_ASSERT(diagnostic.empty(), detail::assert_handler{});
    if (exit_code != 0 && !expect_bad_exit_code)
        throw libclang_error("preprocessor: command '" + cmd + "' exited with non-zero exit code ("
                             + std::to_string(exit_code) + ")");

    return result;
}

clang_preprocess_result clang_preprocess(const libclang_compile_config& c, const char* full_path,
                                         const diagnostic_logger& logger)
{
    if (!std::ifstream(full_path))
        throw libclang_error("preprocessor: file '" + std::string(full_path) + "' doesn't exist");

    // if we're fast preprocessing we only preprocess the main file, not includes
    // this is done by disabling all include search paths when doing the preprocessing
    // to allow macros a separate preprocessing with the -dM flag is done that extracts all macros
    // they are then manually defined before
    auto fast_preprocessing = detail::libclang_compile_config_access::fast_preprocessing(c);

    auto macro_file = fast_preprocessing ? write_macro_file(c, full_path, logger) : "";

    clang_preprocess_result result;
    try
    {
        result = clang_preprocess_impl(c, logger, full_path,
                                       fast_preprocessing ? macro_file.c_str() : nullptr);
    }
    catch (...)
    {
        if (fast_preprocessing)
        {
            auto err = std::remove(macro_file.c_str());
            DEBUG_ASSERT(err == 0, detail::assert_handler{});
        }
        throw;
    }

    if (fast_preprocessing)
    {
        auto err = std::remove(macro_file.c_str());
        DEBUG_ASSERT(err == 0, detail::assert_handler{});
    }

    return result;
}

//==== parsing ===//
class position
{
public:
    position(ts::object_ref<std::string> result, const char* ptr) noexcept
    : result_(result), cur_line_(1u), cur_column_(0u), ptr_(ptr), write_(true)
    {}

    void set_line(unsigned line)
    {
        if (cur_line_ != line)
        {
            *result_ += "#line " + std::to_string(line) + "\n";
            cur_line_   = line;
            cur_column_ = 0;
        }
    }

    void write_str(std::string str)
    {
        if (write_ == false)
            return;
        for (auto c : str)
        {
            *result_ += c;
            ++cur_column_;

            if (c == '\n')
            {
                ++cur_line_;
                cur_column_ = 0;
            }
        }
    }

    void bump() noexcept
    {
        if (write_ == true)
        {
            result_->push_back(*ptr_);
            ++cur_column_;

            if (*ptr_ == '\n')
            {
                ++cur_line_;
                cur_column_ = 0;
            }
        }
        ++ptr_;
    }

    void bump(std::size_t offset) noexcept
    {
        if (write_ == true)
        {
            for (std::size_t i = 0u; i != offset; ++i)
                bump();
        }
        else
            skip(offset);
    }

    // no write, no newline detection
    void skip(std::size_t offset = 1u) noexcept
    {
        ptr_ += offset;
    }

    void skip_with_linecount() noexcept
    {
        if (write_ == true)
        {
            ++cur_column_;
            if (*ptr_ == '\n')
            {
                result_->push_back('\n');
                ++cur_line_;
                cur_column_ = 0;
            }
        }

        ++ptr_;
    }

    void enable_write() noexcept
    {
        write_.set();
    }

    void disable_write() noexcept
    {
        write_.try_reset();
    }

    bool write_enabled() const noexcept
    {
        return write_ == true;
    }

    explicit operator bool() const noexcept
    {
        return *ptr_ != '\0';
    }

    const char* ptr() const noexcept
    {
        return ptr_;
    }

    unsigned cur_line() const noexcept
    {
        return cur_line_;
    }

    unsigned cur_column() const noexcept
    {
        return cur_column_;
    }

    bool was_newl() const noexcept
    {
        return result_->empty() || result_->back() == '\n';
    }

private:
    ts::object_ref<std::string> result_;
    unsigned                    cur_line_, cur_column_;
    const char*                 ptr_;
    ts::flag                    write_;
};

bool starts_with(const position& p, const char* str, std::size_t len)
{
    return std::strncmp(p.ptr(), str, len) == 0;
}

template <std::size_t N>
bool starts_with(const position& p, const char (&str)[N])
{
    return std::strncmp(p.ptr(), str, N - 1) == 0;
}

void skip(position& p, const char* str)
{
    DEBUG_ASSERT(starts_with(p, str, std::strlen(str)), detail::assert_handler{});
    p.skip(std::strlen(str));
}

void bump_spaces(position& p, bool bump = false)
{
    while (starts_with(p, " "))
        if (bump)
            p.bump();
        else
            p.skip();
}

detail::pp_doc_comment parse_c_doc_comment(position& p)
{
    detail::pp_doc_comment result;
    result.kind = detail::pp_doc_comment::c;

    auto indent = p.cur_column() + 3;

    if (starts_with(p, " "))
    {
        // skip one whitespace at most
        p.skip();
        ++indent;
    }

    while (!starts_with(p, "*/"))
    {
        if (starts_with(p, "\n"))
        {
            // remove trailing spaces
            while (!result.comment.empty() && result.comment.back() == ' ')
                result.comment.pop_back();

            // skip newline(s)
            while (starts_with(p, "\n"))
            {
                p.skip_with_linecount();
                result.comment += '\n';
            }

            // skip indentation
            auto actual_indent = 0u;
            for (auto i = 0u; i < indent && starts_with(p, " "); ++i)
            {
                ++actual_indent;
                p.skip();
            }

            auto extra_indent = 0u;
            while (starts_with(p, " "))
            {
                ++extra_indent;
                p.skip();
            }

            // skip continuation star, if any
            if (starts_with(p, "*") && !starts_with(p, "*/"))
            {
                p.skip();
                if (starts_with(p, " "))
                    // skip one whitespace at most
                    p.skip();
            }
            else
            {
                // insert extra indent again
                result.comment += std::string(extra_indent, ' ');
                // use minimum indent in the future
                indent = std::min(actual_indent, indent);
            }
        }
        else
        {
            result.comment += *p.ptr();
            p.skip();
        }
    }
    p.skip(2u);

    // remove trailing star
    if (!result.comment.empty() && result.comment.back() == '*')
        result.comment.pop_back();
    // remove trailing spaces
    while (!result.comment.empty() && result.comment.back() == ' ')
        result.comment.pop_back();

    result.line = p.cur_line();
    return result;
}

bool skip_c_comment(position& p, detail::preprocessor_output& output)
{
    if (!starts_with(p, "/*"))
        return false;
    p.skip(2u);

    if (starts_with(p, "*/"))
        // empty comment
        p.skip(2u);
    else if (p.write_enabled() && (starts_with(p, "*") || starts_with(p, "!")))
    {
        // doc comment
        p.skip();
        output.comments.push_back(parse_c_doc_comment(p));
    }
    else
    {
        while (!starts_with(p, "*/"))
            p.skip_with_linecount();
        p.skip(2u);
    }

    return true;
}

detail::pp_doc_comment parse_cpp_doc_comment(position& p, bool end_of_line)
{
    detail::pp_doc_comment result;
    result.kind = end_of_line ? detail::pp_doc_comment::end_of_line : detail::pp_doc_comment::cpp;
    if (starts_with(p, " "))
        // skip one whitespace at most
        p.skip();

    while (!starts_with(p, "\n"))
    {
        result.comment += *p.ptr();
        p.skip();
    }
    // don't skip newline

    // remove trailing spaces
    while (!result.comment.empty() && result.comment.back() == ' ')
        result.comment.pop_back();
    result.line = p.cur_line();
    return result;
}

bool can_merge_comment(const detail::pp_doc_comment& comment, unsigned cur_line)
{
    return comment.line + 1 == cur_line
           && (comment.kind == detail::pp_doc_comment::cpp
               || comment.kind == detail::pp_doc_comment::end_of_line);
}

void merge_or_add(detail::preprocessor_output& output, detail::pp_doc_comment comment)
{
    if (output.comments.empty() || !can_merge_comment(output.comments.back(), comment.line))
        output.comments.push_back(std::move(comment));
    else
    {
        auto& result = output.comments.back();
        result.comment += "\n" + std::move(comment.comment);
        if (result.kind != detail::pp_doc_comment::end_of_line)
            result.line = comment.line;
    }
}

bool skip_cpp_comment(position& p, detail::preprocessor_output& output)
{
    if (!starts_with(p, "//"))
        return false;
    p.skip(2u);

    if (p.write_enabled() && (starts_with(p, "/") || starts_with(p, "!")))
    {
        // C++ style doc comment
        p.skip();
        auto comment = parse_cpp_doc_comment(p, false);
        merge_or_add(output, std::move(comment));
    }
    else if (p.write_enabled() && starts_with(p, "<"))
    {
        // end of line doc comment
        p.skip();
        auto comment = parse_cpp_doc_comment(p, true);
        output.comments.push_back(std::move(comment));
    }
    else
    {
        auto newline = std::strchr(p.ptr(), '\n');
        p.skip(std::size_t(newline - p.ptr())); // don't skip newline
    }

    return true;
}

std::unique_ptr<cpp_macro_definition> build(std::string name, ts::optional<std::string> args,
                                            std::string rep)
{
    if (!args)
        return cpp_macro_definition::build_object_like(std::move(name), std::move(rep));

    cpp_macro_definition::function_like_builder builder{std::move(name)};
    builder.replacement(std::move(rep));

    auto cur_ptr   = args.value().c_str();
    auto cur_param = cur_ptr;
    while (*cur_ptr)
    {
        while (*cur_ptr && *cur_ptr != ',')
            ++cur_ptr;

        if (*cur_param == '.')
            builder.is_variadic();
        else
            builder.parameter(std::string(cur_param, cur_ptr));

        if (*cur_ptr)
            cur_param = ++cur_ptr;
    }

    return builder.finish();
}

std::unique_ptr<cpp_macro_definition> parse_macro(position& p, detail::preprocessor_output& output)
{
    // format (at new line): #define <name> [replacement]
    // or: #define <name>(<args>) [replacement]
    // note: keep macro definition in file
    if (!p.was_newl() || !starts_with(p, "#define"))
        return nullptr;
    // read line here for comment matching
    auto cur_line = p.cur_line();
    p.bump(std::strlen("#define"));
    bump_spaces(p, true);

    std::string name;
    while (!starts_with(p, "(") && !starts_with(p, " ") && !starts_with(p, "\n"))
    {
        name += *p.ptr();
        p.bump();
    }

    ts::optional<std::string> args;
    if (starts_with(p, "("))
    {
        std::string str;
        for (p.bump(); !starts_with(p, ")"); p.bump())
            str += *p.ptr();
        p.bump();
        args = std::move(str);
    }

    std::string rep;
    auto        in_c_comment = false;
    for (bump_spaces(p, true); in_c_comment || !starts_with(p, "\n"); p.bump())
    {
        if (starts_with(p, "/*"))
            in_c_comment = true;
        else if (in_c_comment && starts_with(p, "*/"))
            in_c_comment = false;
        rep += *p.ptr();
    }
    // don't skip newline

    if (!p.write_enabled())
        return nullptr;

    auto result = build(std::move(name), std::move(args), std::move(rep));
    // match comment directly
    if (!output.comments.empty() && output.comments.back().matches(*result, cur_line))
    {
        result->set_comment(std::move(output.comments.back().comment));
        output.comments.pop_back();
    }
    return result;
}

ts::optional<std::string> parse_undef(position& p)
{
    // format (at new line): #undef <name>
    // due to a clang bug (http://bugs.llvm.org/show_bug.cgi?id=32631) I'll also an undef in the
    // middle of the line
    if (/*!p.was_newl() ||*/ !starts_with(p, "#undef"))
        return ts::nullopt;
    p.bump(std::strlen("#undef"));

    std::string result;
    for (bump_spaces(p, true); !starts_with(p, "\n"); p.bump())
        result += *p.ptr();
    // don't skip newline

    return result;
}

type_safe::optional<detail::pp_include> parse_include(position& p)
{
    // format (at new line, literal <>): #include <filename>
    // or: #include "filename"
    // note: write include back
    if (!p.was_newl() || !starts_with(p, "#include"))
        return type_safe::nullopt;
    p.bump(std::strlen("#include"));
    if (starts_with(p, "_next"))
        p.bump(std::strlen("_next"));
    bump_spaces(p);

    auto include_kind = cpp_include_kind::system;
    auto end_str      = "";
    if (starts_with(p, "\""))
    {
        include_kind = cpp_include_kind::local;
        end_str      = "\"";
    }
    else if (starts_with(p, "<"))
    {
        include_kind = cpp_include_kind::system;
        end_str      = ">";
    }
    else
        DEBUG_UNREACHABLE(detail::assert_handler{});
    p.bump();

    std::string filename;
    for (; !starts_with(p, "\"") && !starts_with(p, ">"); p.bump())
        filename += *p.ptr();
    DEBUG_ASSERT(starts_with(p, end_str, std::strlen(end_str)), detail::assert_handler{},
                 "bad termination");
    p.bump();
    skip(p, " /* clang -E -dI */");
    DEBUG_ASSERT(starts_with(p, "\n"), detail::assert_handler{});
    // don't skip newline

    if (!p.write_enabled())
        return type_safe::nullopt;

    if (filename.size() > 2u && filename[0] == '.' && (filename[1] == '/' || filename[1] == '\\'))
        filename = filename.substr(2);

    return detail::pp_include{std::move(filename), "", include_kind, p.cur_line()};
}

bool bump_pragma(position& p)
{
    // format (at new line): #pragma <stuff..>\n
    if (!p.was_newl() || !starts_with(p, "#pragma"))
        return false;

    while (!starts_with(p, "\n"))
        p.bump();
    // don't skip newline

    return true;
}

struct linemarker
{
    std::string file;
    unsigned    line;
    enum
    {
        line_directive, // no change in file
        enter_new,      // open a new file
        enter_old,      // return to an old file
    } flag
        = line_directive;
    bool is_system = false;
};

ts::optional<linemarker> parse_linemarker(position& p)
{
    // format (at new line): # <line> "<filename>" <flags>
    // flag 1: enter_new
    // flag 2: enter_old
    // flag 3: system file
    // flag 4: ignored
    if (!p.was_newl() || !starts_with(p, "#"))
        return ts::nullopt;
    p.skip();
    DEBUG_ASSERT(!starts_with(p, "define") && !starts_with(p, "undef") && !starts_with(p, "pragma"),
                 detail::assert_handler{}, "handle macros first");

    linemarker result;

    std::string line;
    for (bump_spaces(p); std::isdigit(*p.ptr()); p.skip())
        line += *p.ptr();
    result.line = unsigned(std::stoi(line));

    bump_spaces(p);
    DEBUG_ASSERT(*p.ptr() == '"', detail::assert_handler{});
    p.skip();

    std::string file_name;
    for (; !starts_with(p, "\""); p.skip())
        file_name += *p.ptr();
    p.skip();
    result.file = std::move(file_name);

    for (; !starts_with(p, "\n"); p.skip())
    {
        bump_spaces(p);

        switch (*p.ptr())
        {
        case '1':
            DEBUG_ASSERT(result.flag == linemarker::line_directive, detail::assert_handler{});
            result.flag = linemarker::enter_new;
            break;
        case '2':
            DEBUG_ASSERT(result.flag == linemarker::line_directive, detail::assert_handler{});
            result.flag = linemarker::enter_old;
            break;
        case '3':
            result.is_system = true;
            break;
        case '4':
            break; // ignored

        default:
            DEBUG_UNREACHABLE(detail::assert_handler{}, "invalid line marker");
            break;
        }
    }
    p.skip();

    return result;
}
} // namespace

detail::preprocessor_output detail::preprocess(const libclang_compile_config& config,
                                               const char* path, const diagnostic_logger& logger)
{
    detail::preprocessor_output                  result;
    std::unordered_map<std::string, std::string> indirect_includes;

    auto preprocessed = clang_preprocess(config, path, logger);

    position p(ts::ref(result.source), preprocessed.file.c_str());
    ts::flag in_string(false), in_char(false), first_line(true);
    while (p)
    {
        auto next = std::strpbrk(p.ptr(), R"(\"'#/)"); // look for \, ", ', # or /
        if (next && next > p.ptr())
            p.bump(std::size_t(next - p.ptr() - 1)); // subtract one to get before that character

        if (starts_with(p, R"(\\)")) // starts with two backslashes
            p.bump(2u);
        else if (starts_with(p, "\\\"")) // starts with \"
            p.bump(2u);
        else if (starts_with(p, R"(\")")) // starts with \"
            p.bump(2u);
        else if (starts_with(p, R"(\')")) // starts with \'
            p.bump(2u);
        else if (in_char == false && starts_with(p, R"(")")) // starts with "
        {
            p.bump();
            in_string.toggle();
        }
        else if (in_string == false && starts_with(p, "'"))
        {
            p.bump();
            in_char.toggle();
        }
        else if (in_string == true || in_char == true)
            p.bump();
        else if (auto macro = parse_macro(p, result))
        {
            if (logger.is_verbose())
                logger.log("preprocessor",
                           format_diagnostic(severity::debug,
                                             source_location::make_file(path, p.cur_line()),
                                             "parsing macro '", macro->name(), "'"));

            result.macros.push_back({std::move(macro), p.cur_line()});
        }
        else if (auto undef = parse_undef(p))
        {
            if (p.write_enabled())
            {
                if (logger.is_verbose())
                    logger.log("preprocessor",
                               format_diagnostic(severity::debug,
                                                 source_location::make_file(path, p.cur_line()),
                                                 "undefining macro '", undef.value(), "'"));

                result.macros.erase(std::remove_if(result.macros.begin(), result.macros.end(),
                                                   [&](const pp_macro& e) {
                                                       return e.macro->name() == undef.value();
                                                   }),
                                    result.macros.end());
            }
        }
        else if (auto include = parse_include(p))
        {
            if (p.write_enabled())
            {
                if (logger.is_verbose())
                    logger.log("preprocessor",
                               format_diagnostic(severity::debug,
                                                 source_location::make_file(path, p.cur_line()),
                                                 "parsing include '", include.value().file_name,
                                                 "'"));

                result.includes.push_back(std::move(include.value()));
            }
        }
        else if (bump_pragma(p))
            continue;
        else if (auto lm = parse_linemarker(p))
        {
            if (lm.value().flag == linemarker::enter_new)
            {
                if (p.write_enabled())
                {
                    // this is a direct include, update the full path of the last include
                    // note: path can be empty if pre clang 4 and not fast preprocessing
                    // in this case we can't get the full path at all
                    if (!result.includes.empty())
                    {
                        DEBUG_ASSERT(result.includes.back().full_path.empty()
                                         && lm.value().file.find(result.includes.back().file_name)
                                                != std::string::npos,
                                     detail::assert_handler{});
                        result.includes.back().full_path = lm.value().file;
                    }
                }
                else
                {
                    // this is an indirect include, remember it to get full path for indirect
                    // includes
                    auto& full_path = lm.value().file;

                    auto last_dir  = full_path.find_last_of("/\\");
                    auto file_name = last_dir == std::string::npos
                                         ? full_path
                                         : full_path.substr(last_dir + 1u);

                    indirect_includes.emplace(std::move(file_name), full_path);
                }

                p.disable_write();
            }
            else if (lm.value().flag == linemarker::enter_old)
            {
                if (lm.value().file == path)
                {
                    p.enable_write();
                    p.set_line(lm.value().line);
                }
            }
            else if (lm.value().flag == linemarker::line_directive && p.write_enabled())
            {
                if (first_line.try_reset() && lm.value().file == path && lm.value().line == 1u)
                {
                    // this is the first line marker
                    // just skip all builtin macro stuff until we reach the file again
                    auto closing_line_marker = std::string("# 1 \"") + path + "\" 2\n";

                    auto ptr = std::strstr(p.ptr(), closing_line_marker.c_str());
                    DEBUG_ASSERT(ptr, detail::assert_handler{});
                    p.skip(std::size_t(ptr - p.ptr()));
                    p.skip(closing_line_marker.size());
                }
                else if (lm.value().line + 1 == p.cur_line())
                {
                    // this is a linemarker after a -dI injected include directive
                    // it simply adds the newline we already did
                }
                else
                {
                    DEBUG_ASSERT(lm.value().line >= p.cur_line(), detail::assert_handler{});
                    while (p.cur_line() < lm.value().line)
                        p.write_str("\n");
                }
            }
        }
        else if (skip_c_comment(p, result))
            continue;
        else if (skip_cpp_comment(p, result))
            continue;
        else
            p.bump();
    }

    // get full path for indirect includes
    // doesn't work if fast preprocessing
    if (!detail::libclang_compile_config_access::fast_preprocessing(config))
    {
        for (auto& include : result.includes)
            if (include.full_path.empty())
            {
                auto last_sep = include.file_name.find_last_of("/\\");

                auto iter = indirect_includes.find(last_sep == std::string::npos
                                                       ? include.file_name
                                                       : include.file_name.substr(last_sep + 1u));
                if (iter != indirect_includes.end())
                    include.full_path = iter->second;
                else
                    logger.log("preprocessor",
                               format_diagnostic(severity::warning,
                                                 source_location::make_file(path, include.line),
                                                 "unable to retrieve full path for include '",
                                                 include.file_name,
                                                 "' (please file a bug report)"));
            }
    }

    return result;
}
