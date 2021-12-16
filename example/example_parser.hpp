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

#include <cppast/visitor.hpp> // visit()

inline void handle_ast(const cppast::cpp_file& file)
{
    std::string prefix;
    // visit each entity in the file
    cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (info.event == cppast::visitor_info::container_entity_exit) // exiting an old container
            prefix.pop_back();
        else if (info.event == cppast::visitor_info::container_entity_enter)
        // entering a new container
        {
            std::cout << prefix << "'" << e.name() << "' - " << cppast::to_string(e.kind()) << '\n';
            prefix += "\t";
        }
        else // if (info.event == cppast::visitor_info::leaf_entity) // a non-container entity
            std::cout << prefix << "'" << e.name() << "' - " << cppast::to_string(e.kind()) << '\n';
    });
}

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
    explicit mover_file_parser(type_safe::object_ref<const cpp_entity_index> idx, ThreadPool& pool,
                               Args&&... args)
    : parser_(std::forward<Args>(args)...), idx_(idx), pool(pool)
    {}

    /// \effects Parses the given file using the given configuration.
    /// \returns The parsed file or an empty optional, if a fatal error occurred.
    std::future<void> parse(std::string path, const config& c, const std::string database_dir)
    {
        std::future<void> x = pool.enqueue([path, c, this, database_dir]() {
            std::ifstream ifs(path);
            std::string   content((std::istreambuf_iterator<char>(ifs)),
                                (std::istreambuf_iterator<char>()));
            if (content.find("REGISTER_USER_OP") == std::string::npos)
            {
                parser_.logger().log(log_prefix, diagnostic{"no register found '" + path + "'",
                                                            source_location(), severity::info});
                return;
            }
            parser_.logger().log(log_prefix, diagnostic{"parsing file '" + path + "'",
                                                        source_location(), severity::info});
            try
            {
                auto file = parser_.parse(*idx_, path, c);
                auto ptr  = file.get();
                parser_.logger().log(log_prefix, diagnostic{"done parsing file '" + path + "'",
                                                            source_location(), severity::info});
                handle_ast(*file);
            }
            catch (cppast::libclang_error& ex)
            {
                std::cerr << "fatal libclang error: " << ex.what() << '\n';
            }
        });

        // if (file)
        // files_.push_back(std::move(file));
        // return type_safe::opt_ref(ptr);

        return x;
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
    ThreadPool&                                   pool;
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
        static ThreadPool                     pool(64);
        cppast::libclang_compilation_database database(argv[1]); // the compilation database
        // simple_file_parser allows parsing multiple files and stores the results for us
        using ParserT = cppast::mover_file_parser<cppast::libclang_parser>;
        ParserT                  parser(type_safe::ref(index), pool);
        static const std::string database_dir = std::string(argv[1]);
        using namespace cppast;
        struct data_t
        {
            ParserT&                             parser;
            const libclang_compilation_database& database;
        } data{parser, database};
        static std::vector<std::future<void>> v{};
        detail::for_each_file(database, &data, [](void* ptr, std::string path) {
            using namespace cppast;
            auto&      data                  = *static_cast<data_t*>(ptr);
            const bool is_cpp                = path.find(".cpp") != std::string::npos;
            const bool is_Segmentation_fault = will_cause_Segmentation_fault({}, path);
            const bool is_in_database_dir    = path.find(database_dir) != std::string::npos;
            // const bool is_in_user_op_dir
            // = path.find("oneflow/user/ops/relu_op.cpp") != std::string::npos;
            const bool is_in_user_op_dir = path.find("oneflow/user/ops") != std::string::npos;
            if (!is_cpp || is_in_database_dir || is_Segmentation_fault || !is_in_user_op_dir)
            {
                return;
            }
            cppast::libclang_compile_config config(data.database, path);
            auto f = data.parser.parse(path, std::move(config), std::move(database_dir));
            v.push_back(std::move(f));
        });
        for (auto& f : v)
        {
            f.get();
        }
        // cppast::parse_database(parser, database); // parse all files in the database

        if (parser.error())
            // a non-fatal parse error
            // error has been logged to stderr
            return 1;

        // for (auto& file : parser.files())
        // cb(file);
    }
    std::cerr << "done all\n";
    return 0;
}
catch (std::exception& ex)
{
    std::cerr << ex.what() << '\n';
    return 1;
}

#endif // CPPAST_EXAMPLE_PARSER_HPP_INCLUDED
