// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/parser.hpp>
#include <cppast/detail/parser/istream_lexer.hpp>
#include <iostream>

using namespace cppast;
using namespace cppast::detail::parser;

class print_visitor : public ast::detailed_visitor
{
public:
    void on_node(const ast::terminal_integer& node) override
    {
        print() << "integer literal (" << node.value << ")\n";
    }

    void on_node(const ast::terminal_float& node) override
    {
        print() << "float literal (" << node.value << ")\n";
    }

    void on_node(const ast::terminal_string& node) override
    {
        print() << "string literal (" << node.value << ")\n";
    }

    void on_node(const ast::identifier& node) override
    {
        print() << "identifier (full name: \"" << node.full_qualified_name() << "\")\n";
    }

    void on_node(const ast::expression_invoke& node) override
    {
        print() << "invoke expression (\"" << node.callee->full_qualified_name() << "(<"
            << node.args.size() << " args>)\")\n";
    }

    void on_node(const ast::expression_cpp_attribute& node) override
    {
        print() << "C++ attribute expression (\"" << node.body->callee->full_qualified_name() << "(<"
            << node.body->args.size() << " args>)\")\n";
    }

private:
    std::size_t _depth = 0;

    std::ostream& print()
    {
        return std::cout << std::string(_depth, ' ');
    }

    void on_event(event event) override
    {
        switch(event)
        {
        case event::children_enter:
            _depth++; break;
        case event::children_exit:
            _depth--; break;
        default:
            (void)42;
        }
    }
};

int main(int argc, char** argv)
{
    if(argc == 2)
    {
        std::istringstream input{argv[1]};
        stderr_diagnostic_logger logger;
        logger.set_verbose(true);

        istream_lexer lexer{input, logger};
        parser parser{lexer};

        std::cout << "Parsing \"" << input.str() << "\" as invoke expr...\n";
        std::shared_ptr<ast::node> expr = parser.parse_invoke();

        // Try again with an attribute
        if(expr == nullptr)
        {
            input.str(argv[1]);
            lexer.reset();
            std::cout << "Parsing \"" << input.str() << "\" as C++ attribute expr...\n";
            expr = parser.parse_cpp_attribute();
        }

        if(expr != nullptr)
        {
            std::cout << "ok\n";

            print_visitor visitor;
            expr->visit(visitor);
        }
        else
        {
            std::cout << "error\n";
        }
    }
    else
    {
        std::cerr << "usage: parser_example <intput>";
    }
}
