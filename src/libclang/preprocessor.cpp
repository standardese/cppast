// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "preprocessor.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <cstdio>

// treat the tiny-process-library as header only
#include <process.hpp>
#include <process.cpp>
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include <process_win.cpp>
#else
#include <process_unix.cpp>
#endif

#include <type_safe/flag.hpp>
#include <type_safe/optional.hpp>
#include <type_safe/reference.hpp>

#include <cppast/cpp_entity_kind.hpp>

using namespace cppast;
namespace ts = type_safe;

namespace
{
    // build the command that runs the preprocessor
    std::string get_command(const libclang_compile_config& c, const char* full_path)
    {
        // -E: print preprocessor output
        // -CC: keep comments, even in macro
        // -dD: print macro definitions as well
        // -dI: print include directives as well
        // -Wno-pragma-once-outside-header: hide wrong warning
        std::string cmd(detail::libclang_compile_config_access::clang_binary(c)
                        + " -E -CC -dD -dI -Wno-pragma-once-outside-header ");

        // add other flags
        for (auto& flag : detail::libclang_compile_config_access::flags(c))
        {
            cmd += flag;
            cmd += ' ';
        }

        // add path to file being processed
        cmd += full_path;

        return cmd;
    }

    // gets the full preprocessor output
    std::string get_full_preprocess_output(const libclang_compile_config& c, const char* full_path)
    {
        std::string preprocessed;

        auto    cmd = get_command(c, full_path);
        Process process(cmd, "",
                        [&](const char* str, std::size_t n) {
                            preprocessed.reserve(preprocessed.size() + n);
                            for (auto end = str + n; str != end; ++str)
                                if (*str != '\r')
                                    preprocessed.push_back(*str);
                        },
                        [&](const char* str, std::size_t n) {
                            std::fprintf(stderr, "%.*s\n", static_cast<int>(n),
                                         str); // TODO: log error properly
                        });

        auto exit_code = process.get_exit_status();
        if (exit_code != 0)
            throw libclang_error("preprocessor: command '" + cmd
                                 + "' exited with non-zero exit code (" + std::to_string(exit_code)
                                 + ")");

        return preprocessed;
    }

    class position
    {
    public:
        position(ts::object_ref<std::string> result, const char* ptr) noexcept
        : result_(result), cur_line_(1u), ptr_(ptr), write_(true)
        {
        }

        void write_str(std::string str)
        {
            if (write_ == false)
                return;
            for (auto c : str)
            {
                *result_ += c;
                if (c == '\n')
                    ++cur_line_;
            }
        }

        void bump() noexcept
        {
            if (write_ == true)
            {
                result_->push_back(*ptr_);
                if (*ptr_ == '\n')
                    ++cur_line_;
            }
            ++ptr_;
        }

        void bump(std::size_t offset) noexcept
        {
            for (std::size_t i = 0u; i != offset; ++i)
                bump();
        }

        // no write, no newline detection
        void skip(std::size_t offset = 1u) noexcept
        {
            ptr_ += offset;
        }

        void enable_write() noexcept
        {
            write_.set();
        }

        void disable_write() noexcept
        {
            write_.try_reset();
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

        bool was_newl() const noexcept
        {
            return result_->empty() || result_->back() == '\n';
        }

    private:
        ts::object_ref<std::string> result_;
        unsigned                    cur_line_;
        const char*                 ptr_;
        ts::flag                    write_;
    };

    bool starts_with(const position& p, const char* str)
    {
        return std::strncmp(p.ptr(), str, std::strlen(str)) == 0;
    }

    bool bump_c_str(position& p)
    {
        if (!starts_with(p, "/*"))
            return false;
        p.bump(2u);

        while (!starts_with(p, "*/"))
            p.bump();
        p.bump(2u);
        return true;
    }

    bool bump_cpp_str(position& p)
    {
        if (!starts_with(p, "//"))
            return false;
        p.bump(2u);

        while (!starts_with(p, "\n"))
            p.bump();
        return true;
    }

    void skip_spaces(position& p)
    {
        while (starts_with(p, " "))
            p.skip();
    }

    std::unique_ptr<cpp_macro_definition> parse_macro(position& p)
    {
        // format (at new line): #define <name> [replacement]
        // or: #define <name>(<args>) [replacement]
        if (!p.was_newl() || !starts_with(p, "#define"))
            return nullptr;
        p.skip(std::strlen("#define"));
        skip_spaces(p);

        std::string name;
        while (!starts_with(p, "(") && !starts_with(p, " ") && !starts_with(p, "\n"))
        {
            name += *p.ptr();
            p.skip();
        }

        ts::optional<std::string> args;
        if (starts_with(p, "("))
        {
            std::string str;
            for (p.skip(); !starts_with(p, ")"); p.skip())
                str += *p.ptr();
            p.skip();
            args = std::move(str);
        }

        std::string rep;
        for (skip_spaces(p); !starts_with(p, "\n"); p.skip())
            rep += *p.ptr();
        // don't skip newline

        return cpp_macro_definition::build(std::move(name), std::move(args), std::move(rep));
    }

    ts::optional<std::string> parse_undef(position& p)
    {
        // format (at new line): #undef <name>
        if (!p.was_newl() || !starts_with(p, "#undef"))
            return ts::nullopt;
        p.skip(std::strlen("#undef"));

        std::string result;
        for (skip_spaces(p); !starts_with(p, "\n"); p.skip())
            result += *p.ptr();
        // don't skip newline

        return result;
    }

    std::unique_ptr<cpp_include_directive> parse_include(position& p)
    {
        // format (at new line, literal <>): #include <filename>
        // or: #include "filename"
        if (!p.was_newl() || !starts_with(p, "#include"))
            return nullptr;
        p.skip(std::strlen("#include"));
        skip_spaces(p);

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
        p.skip();

        std::string filename;
        for (; !starts_with(p, "\"") && !starts_with(p, ">"); p.skip())
            filename += *p.ptr();
        DEBUG_ASSERT(starts_with(p, end_str), detail::assert_handler{}, "bad termination");
        p.skip();
        DEBUG_ASSERT(starts_with(p, "\n"), detail::assert_handler{});
        // don't skip newline

        return cpp_include_directive::build(cpp_file_ref(cpp_entity_id(filename), filename),
                                            include_kind);
    }

    bool skip_pragma(position& p)
    {
        // format (at new line): #pragma <stuff..>\n
        if (!p.was_newl() || !starts_with(p, "#pragma"))
            return false;

        while (!starts_with(p, "\n"))
            p.skip();
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
        } flag         = line_directive;
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
        DEBUG_ASSERT(!starts_with(p, "define") && !starts_with(p, "undef")
                         && !starts_with(p, "pragma"),
                     detail::assert_handler{}, "handle macros first");

        linemarker result;

        std::string line;
        for (skip_spaces(p); std::isdigit(*p.ptr()); p.skip())
            line += *p.ptr();
        result.line = unsigned(std::stoi(line));

        skip_spaces(p);
        DEBUG_ASSERT(*p.ptr() == '"', detail::assert_handler{});
        p.skip();

        std::string file_name;
        for (; !starts_with(p, "\""); p.skip())
            file_name += *p.ptr();
        p.skip();
        result.file = std::move(file_name);

        for (; !starts_with(p, "\n"); p.skip())
        {
            skip_spaces(p);

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
}

detail::preprocessor_output detail::preprocess(const libclang_compile_config& config,
                                               const char*                    path)
{
    detail::preprocessor_output result;

    auto output = get_full_preprocess_output(config, path);

    position    p(ts::ref(result.source), output.c_str());
    std::size_t file_depth = 0u;
    while (p)
    {
        if (auto macro = parse_macro(p))
        {
            if (file_depth == 0u)
                result.entities.push_back({std::move(macro), p.cur_line()});
        }
        else if (auto undef = parse_undef(p))
        {
            if (file_depth == 0u)
                result.entities
                    .erase(std::remove_if(result.entities.begin(), result.entities.end(),
                                          [&](const pp_entity& e) {
                                              return e.entity->kind()
                                                         == cpp_entity_kind::macro_definition_t
                                                     && e.entity->name() == undef.value();
                                          }),
                           result.entities.end());
        }
        else if (auto include = parse_include(p))
        {
            if (file_depth == 0u)
                result.entities.push_back({std::move(include), p.cur_line()});
        }
        else if (skip_pragma(p))
            continue;
        else if (auto lm = parse_linemarker(p))
        {
            switch (lm.value().flag)
            {
            case linemarker::line_directive:
                break; // ignore
            // no need to handle it, preprocessed output doesn't need to match line numbers precisely

            case linemarker::enter_new:
                if (file_depth == 0u && lm.value().file.front() != '<')
                {
                    // this file is directly included by the given file
                    // write include with full path
                    // note: don't build include here, do it when an #include is encountered
                    p.write_str("#include \"" + lm.value().file + "\"\n");
                }

                ++file_depth;
                p.disable_write();
                break;

            case linemarker::enter_old:
                --file_depth;
                if (file_depth == 0u)
                {
                    DEBUG_ASSERT(lm.value().file == path, detail::assert_handler{});
                    p.enable_write();
                }
                break;
            }
        }
        else if (bump_c_str(p))
            // write an additional newline after each string
            // this allows matching documentation comments to entities generated from macros
            // as the entity corresponding to the documentation comment will be on the next line
            // otherwise all entities would have the same line number
            p.write_str("\n");
        else if (bump_cpp_str(p))
            continue;
        else
            p.bump();
    }

    return result;
}
