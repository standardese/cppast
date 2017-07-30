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

std::shared_ptr<ast::node> parse(const std::string& input, diagnostic_logger& logger)
{
    std::istringstream istream{input};
    logger.set_verbose(true);
    istream_lexer lexer{istream, logger};
    parser parser{lexer};

    return parser.parse_cpp_attribute();
}

int main(int argc, char** argv)
{
    if(argc == 2)
    {
        stderr_diagnostic_logger logger;
        auto attribute = parse(argv[1], logger);

        if(attribute != nullptr)
        {
            std::cout << "ok\n";

            print_visitor visitor;
            attribute->visit(visitor);
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
