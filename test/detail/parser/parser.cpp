#include <cppast/detail/parser/parser.hpp>
#include <cppast/detail/parser/fake_lexer.hpp>
#include <catch.hpp>
#include "logger_mock.hpp"
#include "token_generator.hpp"

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
};

class parser_context : public cppast::test::logger_context
{
private:
    cppast::test::token_generator token_generator;
public:
    cppast::detail::parser::fake_lexer lexer;
    test_parser parser;

    parser_context(const std::vector<token_kind>& kinds) :
        logger_context{true},
        lexer{token_generator.random_tokens(kinds), logger},
        parser{lexer}
    {}
};

TEST_CASE("the expression parser parses terminals")
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
