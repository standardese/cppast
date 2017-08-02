#include <catch.hpp>
#include <trompeloeil.hpp>
#include <cppast/detail/parser/istream_lexer.hpp>

using namespace trompeloeil;

class diagnostic_logger_mock_base : public cppast::diagnostic_logger
{
private:
    bool do_log(const char* source, const cppast::diagnostic& d) const override final
    {
        diagnostic_logged(
            source,
            d.severity,
            d.location.file.value_or(""),
            d.location.line.value_or(0),
            d.location.column.value_or(0),
            d.message
        );

        return true;
    }

    virtual void diagnostic_logged(
        const std::string& source,
        cppast::severity severity,
        const std::string& file,
        std::size_t line,
        std::size_t column,
        const std::string& message
    ) const = 0;
};

class diagnostic_logger_mock : public diagnostic_logger_mock_base
{
public:
    MAKE_CONST_MOCK6(
        diagnostic_logged,
        void(const std::string&, cppast::severity, const std::string&, std::size_t, std::size_t, const std::string&),
        override final);
};

struct istream_lexer_context
{
    diagnostic_logger_mock logger;
    const std::string filename;
    const std::string input;
    std::istringstream stream;
    cppast::detail::parser::istream_lexer lexer;
    std::vector<cppast::detail::parser::token> tokens;

    istream_lexer_context(const std::string& input) :
        filename{"file.cpp"},
        input{input},
        stream{input},
        lexer{stream, logger, filename}
    {}

    const std::vector<cppast::detail::parser::token>& read_all()
    {
        tokens.clear();

        while(lexer.read_next_token())
        {
            tokens.push_back(lexer.current_token());
        }

        return tokens;
    }

    bool equal_tokens(const std::vector<std::pair<cppast::detail::parser::token::token_kind, std::string>>& expected)
    {
        read_all();

        if(tokens.size() != expected.size())
        {
            return false;
        }

        for(std::size_t i = 0; i < tokens.size(); ++i)
        {
            if(tokens[i].kind != expected[i].first ||
               tokens[i].token != expected[i].second)
            {
                return false;
            }
        }

        return true;
    }
};

TEST_CASE("istream_lexer properties after construction", "[istream_lexer]")
{
    SECTION("the default token is unknown and pointing out of the input")
    {
        istream_lexer_context context{"foobar"};
        CHECK(context.lexer.current_token().kind == cppast::detail::parser::token::token_kind::unknown);
        CHECK(context.lexer.current_token().token == "");
        CHECK(context.lexer.current_token().line == 0);
        CHECK(context.lexer.current_token().column == 0);
    }

    SECTION("initialized with non-empty input the stream could be read")
    {
        istream_lexer_context context{"foobar"};
        CHECK_FALSE(context.lexer.eof());
    }

    SECTION("initialized with empty input it cannot read anything")
    {
        istream_lexer_context context{""};
        CHECK_FALSE(context.lexer.read_next_token());
        CHECK(context.lexer.eof());
    }
}

TEST_CASE("istream_lexer reads numeric literals", "[istream_lexer]")
{
    SECTION("spaces are skipped")
    {
        istream_lexer_context context{"    1234   "};
        REQUIRE(context.lexer.read_next_token());
        CHECK(context.lexer.current_token().token == "1234");
        CHECK(context.lexer.current_token().kind == cppast::detail::parser::token::token_kind::int_iteral);
        CHECK_FALSE(context.lexer.read_next_token());
    }

    SECTION("-/+ operators are interpreted as part of a literal")
    {
        istream_lexer_context context{"+42 -42"};
        CHECK(context.equal_tokens({
            {cppast::detail::parser::token::token_kind::int_iteral, "+42"},
            {cppast::detail::parser::token::token_kind::int_iteral, "-42"}
        }));

        CHECK(context.tokens[0].int_value() ==  42);
        CHECK(context.tokens[1].int_value() == -42);
    }

    SECTION("Multiple -/+ operators in a literal gives an error")
    {
        auto test = [&](const std::string& signs)
        {
            istream_lexer_context context{signs + "42"};

            REQUIRE_CALL(context.logger, diagnostic_logged(
                "lexer",
                cppast::severity::error,
                context.filename,
                _,_,
                "Unexpected '" + signs.substr(1, 1) + "' character after \"" + signs.substr(0, 1) + "\""
            ));

            context.lexer.read_next_token();
        };

        test("--");
        test("-+");
        test("+-");
        test("++");
    }

    SECTION("A dot in the numeric literal makes the lexer interpret the number as a float literal")
    {
        istream_lexer_context context{"3.141592654"};
        REQUIRE(context.lexer.read_next_token());
        CHECK(context.lexer.current_token().kind == cppast::detail::parser::token::token_kind::float_literal);
        CHECK(context.lexer.current_token().token == "3.141592654");
        CHECK(context.lexer.current_token().float_value() == 3.141592654);
    }

    SECTION("Multiple floating point dots gives an error")
    {
        auto test = [&](const std::string& input)
        {
            istream_lexer_context context{input};

            REQUIRE_CALL(context.logger, diagnostic_logged(
                "lexer",
                cppast::severity::error,
                context.filename,
                _,_,
                "Unexpected floating point dot after \"" + input.substr(0, input.find_last_of(".")) + "\""
            ));

            context.lexer.read_next_token();
        };

        test("3.1415.92654");
        test("..3141592654");
        test(".3.141592654");
    }

    SECTION("-/+ operators are interpreted as part of a float literal")
    {
        istream_lexer_context context{"+42.0 -42.0"};
        CHECK(context.equal_tokens({
            {cppast::detail::parser::token::token_kind::float_literal, "+42.0"},
            {cppast::detail::parser::token::token_kind::float_literal, "-42.0"}
        }));

        CHECK(context.tokens[0].float_value() ==  42.0);
        CHECK(context.tokens[1].float_value() == -42.0);
    }
}

TEST_CASE("istream_lexer reads string literals", "[istream_lexer]")
{
    SECTION("A string literal token includes the enclosing quotes")
    {
        istream_lexer_context context{"\"hello\""};
        REQUIRE(context.lexer.read_next_token());
        CHECK(context.lexer.current_token().line == 0);
        CHECK(context.lexer.current_token().column == 0);
        CHECK(context.lexer.current_token().token == "\"hello\"");
        CHECK(context.lexer.current_token().string_value() == "hello");
    }

    SECTION("An unclosed string literal gives an error")
    {
        istream_lexer_context context{"\"hello 42"};

        REQUIRE_CALL(context.logger, diagnostic_logged(
            "lexer",
            cppast::severity::error,
            context.filename,
            _, _,
            "Expected closing '\"' after \"\"hello 42\""
        ));

        CHECK_FALSE(context.lexer.read_next_token());
    }
}
