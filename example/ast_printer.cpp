// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// This is a very primitive version of the cppast tool.
///
/// Given an input file it will print the AST.

#include <cppast/visitor.hpp> // visit()

#include "example_parser.hpp"

void print_ast(const cppast::cpp_file& file)
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

int main(int argc, char* argv[])
{
    return example_main(argc, argv, {}, &print_ast);
}
