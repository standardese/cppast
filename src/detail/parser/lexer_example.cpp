#include <cppast/detail/parser/lexer.hpp>
#include <cppast/detail/parser/buffered_lexer.hpp>
#include <sstream>
#include <iostream>

using namespace cppast::detail::parser;

int main(int argc, char** argv)
{
    if(argc == 2)
    {
        std::istringstream input{argv[1]};
        buffered_lexer lex{input, 4};

        while(lex.read_next_token())
        {
            std::cout << lex.current_token() << "\n";

            for(std::size_t i = 0; i < lex.buffer_size(); ++i)
            {
                std::cout << "  (next " << i << ": " << lex.next_token(i) << "\n";
            }
        }

        if(!lex.good())
        {
            std::cerr << "ERROR: " << lex.error().value().message << "\n";
        }
    }
    else
    {
        std::cerr << "Use: lexer_example <input>\n";
        return 1;
    }
}
