// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// This is a very primitive version of the cppast tool.

#include <iostream>

#include <cppast/libclang_parser.hpp> // libclang_compilation_database, libclang_parser, etc.
#include <cppast/visitor.hpp>         // visit()

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " <compile-commands-json-dir>\n";
        return 1;
    }
    else
    {
        cppast::libclang_compilation_database database(argv[1]); // the compilation database
        cppast::cpp_entity_index              index; // only needed for cross referencing in the AST

        // simple_file_parser allows parsing multiple files and stores the results for us
        cppast::simple_file_parser<cppast::libclang_parser> parser(type_safe::ref(index));
        try
        {
            cppast::parse_database(parser, database); // parse all files in the database
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

        for (auto& file : parser.files()) // for each file in the project
        {
            std::string prefix;
            // visit each entity in the file
            cppast::visit(file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
                if (info.event
                    == cppast::visitor_info::container_entity_exit) // exiting an old container
                    prefix.pop_back();
                else if (info.event == cppast::visitor_info::container_entity_enter)
                // entering a new container
                {
                    std::cout << prefix << "'" << e.name() << "' - " << cppast::to_string(e.kind())
                              << '\n';
                    prefix += "\t";
                }
                else // if (info.event == cppast::visitor_info::leaf_entity) // a non-container entity
                    std::cout << prefix << "'" << e.name() << "' - " << cppast::to_string(e.kind())
                              << '\n';

                return true; // continue with visit
            });
        }
    }
}
