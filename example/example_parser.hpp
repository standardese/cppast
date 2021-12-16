// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_EXAMPLE_PARSER_HPP_INCLUDED
#define CPPAST_EXAMPLE_PARSER_HPP_INCLUDED

#include <cppast/libclang_parser.hpp>
#include <fstream>
#include <iostream>
#include <string>

// reads the database directory from the command line argument
// parses all files in that directory
// and invokes the callback for each of them
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
        using ParserT = cppast::simple_file_parser<cppast::libclang_parser>;
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
                auto&      data               = *static_cast<data_t*>(ptr);
                const bool is_cpp             = file.find(".cpp") != std::string::npos;
                const bool is_in_database_dir = file.find(database_dir) != std::string::npos;
                const bool is_in_user_op_dir  = file.find("oneflow/user/ops") != std::string::npos;
                if (!is_cpp || is_in_database_dir)
                    return;
                if (!is_in_user_op_dir)
                    return;
                libclang_compile_config config(data.database, file);
                std::ifstream           ifs(file);
                std::string             content((std::istreambuf_iterator<char>(ifs)),
                                    (std::istreambuf_iterator<char>()));
                if (content.find("REGISTER_USER_OP") != std::string::npos)
                {
                    std::cerr << "found: " << file << "\n";
                }
                else
                {
                    std::cerr << "skip: " << file << "\n";
                }
                // data.parser.parse(std::move(file), std::move(config));
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
