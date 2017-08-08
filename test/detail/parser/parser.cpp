// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/parser.hpp>
#include <cppast/detail/parser/fake_lexer.hpp>
#include <cppast/detail/parser/istream_lexer.hpp>
#include <catch.hpp>
#include "logger_mock.hpp"
#include "token_generator.hpp"
#include "syntax_generator.hpp"

using token_kind = cppast::detail::parser::token::token_kind;
namespace ast = cppast::detail::parser::ast;

class test_parser : public cppast::detail::parser::parser
{
public:
    using cppast::detail::parser::parser::parser;

    std::shared_ptr<ast::node> do_parse_expression() override final
    {
        return cppast::detail::parser::parser::do_parse_expression();
    }

    std::shared_ptr<ast::identifier> do_parse_identifier() override final
    {
        return cppast::detail::parser::parser::do_parse_identifier();
    }

    std::pair<bool, std::vector<std::shared_ptr<ast::node>>> do_parse_arguments(
        token_kind open_delim = token_kind::paren_open,
        token_kind close_delim = token_kind::paren_close) override final
    {
        return cppast::detail::parser::parser::do_parse_arguments(open_delim, close_delim);
    }
};

class parser_context : public cppast::test::logger_context
{
private:
    cppast::test::token_generator token_generator;
    cppast::test::syntax_generator syntax_generator;

public:
    cppast::detail::parser::fake_lexer lexer;
    test_parser parser;
    std::shared_ptr<cppast::detail::parser::ast::node> expected_node;

    parser_context(const std::vector<token_kind>& kinds) :
        lexer{token_generator.random_tokens(kinds), logger},
        parser{lexer}
    {}

    template<typename Node>
    parser_context(const cppast::test::syntax_generator::syntax<Node>& syntax) :
        lexer{syntax.second, logger},
        parser{lexer},
        expected_node{syntax.first, syntax.first.get()}
    {}

    parser_context(const cppast::test::syntax_generator::invoke_args_syntax& syntax) :
        lexer{syntax.second, logger},
        parser{lexer}
    {}
};

TEST_CASE("the expression parser parses terminals", "[parser]")
{
    SECTION("parses string literals")
    {
        parser_context context{{token_kind::string_literal}};
        auto node = context.parser.do_parse_expression();
        REQUIRE(node != nullptr);
        auto terminal = ast::node_cast<ast::terminal_string>(node);

        REQUIRE(terminal != nullptr);
        CHECK(terminal->value == context.lexer.tokens()[0].string_value());
    }

    SECTION("parses integer literals")
    {
        parser_context context{{token_kind::int_iteral}};
        auto node = context.parser.do_parse_expression();
        REQUIRE(node != nullptr);
        auto terminal = ast::node_cast<ast::terminal_integer>(node);

        REQUIRE(terminal != nullptr);
        CHECK(terminal->value == context.lexer.tokens()[0].int_value());
    }

    SECTION("parses floating point literals")
    {
        parser_context context{{token_kind::float_literal}};
        auto node = context.parser.do_parse_expression();
        REQUIRE(node != nullptr);
        auto terminal = ast::node_cast<ast::terminal_float>(node);

        REQUIRE(terminal != nullptr);
        CHECK(terminal->value == context.lexer.tokens()[0].float_value());
    }
}

TEST_CASE("the expression parser parses identifiers", "[parser]")
{
    cppast::test::syntax_generator syntax_generator;

    auto test = [](const cppast::test::syntax_generator::syntax<ast::identifier>& syntax)
    {
        parser_context context{syntax};
        auto node = context.parser.do_parse_identifier();
        REQUIRE(node != nullptr);
        auto identifier = ast::node_cast<ast::identifier>(node);

        REQUIRE(identifier != nullptr);
        CHECK_THAT(identifier->scope_names, Catch::Equals(context.expected_node->as<ast::identifier>().scope_names));
        CHECK(identifier->full_qualified_name() == context.expected_node->as<ast::identifier>().full_qualified_name());
    };

    SECTION("parses simple identifiers")
    {
        test(syntax_generator.random_identifier());
    }

    SECTION("parses simple full qualified identifiers")
    {
        test(syntax_generator.random_full_qualified_identifier());
    }

    SECTION("parses complex identifiers")
    {
        for(std::size_t i = 2; i <= 4096; i *= 2)
        {
            SECTION("parses identifiers with " + std::to_string(i) + " nested scopes")
            {
                test(syntax_generator.random_identifier(i));
            }
        }
    }

    SECTION("parses complex full qualified identifiers")
    {
        for(std::size_t i = 2; i <= 4096; i *= 2)
        {
            SECTION("parses full qualified identifiers with " + std::to_string(i) + " nested scopes")
            {
                test(syntax_generator.random_full_qualified_identifier(i));
            }
        }
    }
}

TEST_CASE("the expression parser parses invoke expression arguments", "[parser]")
{
    cppast::test::syntax_generator syntax_generator;

    auto test = [](const cppast::test::syntax_generator::invoke_args_syntax& syntax)
    {
        parser_context context{syntax};
        auto result = context.parser.do_parse_arguments();

        REQUIRE(result.first);
        REQUIRE(result.second.size() == syntax.first.size());

        for(std::size_t i = 0; i < syntax.first.size(); ++i)
        {
            std::function<void(const std::shared_ptr<ast::node>&, const std::shared_ptr<ast::node>&)> node_test =
                [&](const std::shared_ptr<ast::node>& result_node, const std::shared_ptr<ast::node>& expected_node)
            {
                REQUIRE(result_node != nullptr);
                REQUIRE(result_node->kind == expected_node->kind);
                bool visited = false;

                ast::visit_node(*expected_node, cppast::detail::utils::overloaded_function(
                    [&](const ast::terminal_string& str)
                    {
                        CHECK(str.value == expected_node->as<ast::terminal_string>().value);
                        visited = true;
                    },
                    [&](const ast::terminal_integer& i)
                    {
                        CHECK(i.value == expected_node->as<ast::terminal_integer>().value);
                        visited = true;
                    },
                    [&](const ast::terminal_float& f)
                    {
                        CHECK(f.value == expected_node->as<ast::terminal_float>().value);
                        visited = true;
                    },
                    [&](const ast::expression_invoke& i)
                    {
                        CHECK(i.callee->full_qualified_name() == expected_node->as<ast::expression_invoke>().callee->full_qualified_name());
                        REQUIRE(i.args.size() == expected_node->as<ast::expression_invoke>().args.size());

                        for(std::size_t j = 0; j < i.args.size(); ++j)
                        {
                            node_test(i.args[j], expected_node->as<ast::expression_invoke>().args[j]);
                        }
                        visited = true;
                    },
                    [](const auto&){}
                ));

                CHECK(visited);
            };

            node_test(result.second[i], syntax.first[i]);
        }
    };

    auto section = [&](std::size_t depth)
    {
        for(std::size_t i = 0; i < 10; ++i)
        {
            std::size_t count = std::pow(2, i);
            std::ostringstream os;

            os << count << " invoke arguments with up to "
               << depth << " recursively-nested invoke expressions";

            SECTION(os.str())
            {
                test(syntax_generator.random_invoke_args(count, depth));
            }
        }
    };

    SECTION("parses simple invoke args (no nested calls)")
    {
        section(0);
    }

    SECTION("parses nested invoke args")
    {
        section(1);
        section(4);
        section(16);
        section(64);
    }

    SECTION("error checking on ill-formed input")
    {
        auto test = [](const std::string& input, const std::string& error_source, const std::string& expected_error)
        {
            std::istringstream is{input};
            cppast::test::logger_context logger_context;
            cppast::detail::parser::istream_lexer lexer{is, logger_context.logger};
            test_parser parser{lexer};

            REQUIRE_CALL(logger_context.logger, diagnostic_logged(
                error_source,
                cppast::severity::error,
                trompeloeil::_, // file
                trompeloeil::_, // line
                trompeloeil::_, // column
                expected_error
            ));

            auto result = parser.do_parse_arguments();
            CHECK_FALSE(result.first);
        };

        SECTION("missing opening paren returns an error")
        {
            test("foo", "parser.invoke_args", "Expected 'paren_open'");
        }

        SECTION("missing closing paren returns an error")
        {
            test("(foo(), bar(), quux()", "parser.invoke_args", "Expected 'paren_close' or comma (','), but no more tokens are available");
        }

        SECTION("missing comma returns an error")
        {
            test("(foo(), bar(), quux() 42)", "parser.invoke_args", "Expected 'paren_close' or comma (','), got \"42\"");
        }
    }
}
