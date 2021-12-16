// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_EXAMPLE_PARSER_HPP_INCLUDED
#define CPPAST_EXAMPLE_PARSER_HPP_INCLUDED

#include "ThreadPool.h"
#include <cppast/libclang_parser.hpp>
#include <fstream>
#include <iostream>
#include <string>

static ThreadPool pool(60);
static const auto log_prefix = "mover";
namespace cppast
{
/// A simple `FileParser` that parses all files synchronously.
///
/// More advanced parsers could use a thread pool, for example.
template <class Parser>
class mover_file_parser
{
    static_assert(std::is_base_of<cppast::parser, Parser>::value,
                  "Parser must be derived from cppast::parser");

public:
    using parser = Parser;
    using config = typename Parser::config;

    /// \effects Creates a file parser populating the given index
    /// and using the parser created by forwarding the given arguments.
    template <typename... Args>
    explicit mover_file_parser(type_safe::object_ref<const cpp_entity_index> idx, Args&&... args)
    : parser_(std::forward<Args>(args)...), idx_(idx)
    {}

    /// \effects Parses the given file using the given configuration.
    /// \returns The parsed file or an empty optional, if a fatal error occurred.
    type_safe::optional_ref<const cpp_file> parse(std::string path, const config& c,
                                                  const std::string database_dir)
    {
        pool.enqueue([path, c, this, database_dir]() {
            std::ifstream ifs(path);
            std::string   content((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));
            if (content.find("REGISTER") == std::string::npos)
            {
                parser_.logger().log(log_prefix, diagnostic{"no REGISTER found '" + path + "'",
                                                            source_location(), severity::info});
                return;
            }
            parser_.logger().log(log_prefix, diagnostic{"parsing file '" + path + "'",
                                                        source_location(), severity::info});
            auto file = parser_.parse(*idx_, path, c);
            auto ptr  = file.get();
            parser_.logger().log(log_prefix, diagnostic{"done parsing file '" + path + "'",
                                                        source_location(), severity::info});
        });
        // if (file)
        // files_.push_back(std::move(file));
        // return type_safe::opt_ref(ptr);
        return type_safe::nullopt;
    }

    /// \returns The result of [cppast::parser::error]().
    bool error() const noexcept
    {
        return parser_.error();
    }

    /// \effects Calls [cppast::parser::reset_error]().
    void reset_error() noexcept
    {
        parser_.reset_error();
    }

    /// \returns The index that is being populated.
    const cpp_entity_index& index() const noexcept
    {
        return *idx_;
    }

    /// \returns An iteratable object iterating over all the files that have been parsed so far.
    /// \exclude return
    detail::iteratable_intrusive_list<cpp_file> files() const noexcept
    {
        return type_safe::ref(files_);
    }

private:
    Parser                                        parser_;
    detail::intrusive_list<cpp_file>              files_;
    type_safe::object_ref<const cpp_entity_index> idx_;
};

} // namespace cppast

// reads the database directory from the command line argument
// parses all files in that directory
// and invokes the callback for each of them

inline bool will_cause_Segmentation_fault(std::vector<std::string> fault_paths, std::string path)
{
    for (auto fp : fault_paths)
    {
        if (path.find(fp) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

template <typename DATA_T>
void handle_one(void* ptr, std::string path, const std::string database_dir)
{
    using namespace cppast;
    auto&      data                  = *static_cast<DATA_T*>(ptr);
    const bool is_cpp                = path.find(".cpp") != std::string::npos;
    const bool is_Segmentation_fault = will_cause_Segmentation_fault({}, path);
    const bool is_in_database_dir    = path.find(database_dir) != std::string::npos;
    const bool is_in_user_op_dir     = path.find("oneflow/user/ops") != std::string::npos;
    if (!is_cpp || is_in_database_dir || is_Segmentation_fault || !is_in_user_op_dir)
    {
        return;
    }
    cppast::libclang_compile_config config(data.database, path);
    data.parser.parse(std::move(path), std::move(config), std::move(database_dir));
}

template <typename Callback>
int example_main(int argc, char* argv[], const cppast::cpp_entity_index& index, Callback cb)
try
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <compile-commands-json-dir>\n";
        return 1;
    }
    else
    {
        cppast::libclang_compilation_database database(argv[1]); // the compilation database
        // simple_file_parser allows parsing multiple files and stores the results for us
        using ParserT = cppast::mover_file_parser<cppast::libclang_parser>;
        ParserT                  parser(type_safe::ref(index));
        static const std::string database_dir = std::string(argv[1]);
        try
        {
            using namespace cppast;
            struct data_t
            {
                ParserT&                             parser;
                const libclang_compilation_database& database;
            } data{parser, database};
            detail::for_each_file(database, &data, [](void* ptr, std::string file) {
                handle_one<data_t>(ptr, file, database_dir);
            });
            // cppast::parse_database(parser, database); // parse all files in the database
        }
        catch (cppast::libclang_error& ex)
        {
            std::cerr << "fatal libclang error: " << ex.what() << '\n';
            return 1;
        }

        if (parser.error())
            // a non-fatal parse error
            // error has been logged to stderr
            return 1;

        for (auto& file : parser.files())
            cb(file);
    }

    return 0;
}
catch (std::exception& ex)
{
    std::cerr << ex.what() << '\n';
    return 1;
}

#endif // CPPAST_EXAMPLE_PARSER_HPP_INCLUDED
