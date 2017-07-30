// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/istream_lexer.hpp>
#include <sstream>
#include <iostream>

using namespace cppast;
using namespace cppast::detail::parser;

int main(int argc, char** argv)
{
    if(argc == 2)
    {
        std::istringstream input{argv[1]};
        stderr_diagnostic_logger logger;
        istream_lexer lex{input, logger};

        while(lex.read_next_token())
        {
            std::cout << lex.current_token() << "\n";
        }

        if(lex.good())
        {
            std::cout << "Ok\n";
        }
        else
        {
            std::cout << "Error\n";
        }
    }
    else
    {
        std::cerr << "Use: lexer_example <input>\n";
        return 1;
    }
}
