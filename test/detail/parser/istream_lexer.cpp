#include <catch.hpp>
#include <trompeloeil.hpp>
#include <cppast/detail/parser/istream_lexer.hpp>
#include <iostream>
#include <random>

using namespace trompeloeil;

class diagnostic_logger_mock_base : public cppast::diagnostic_logger
{
public:
    // Optionally log to stdout for debugging
    diagnostic_logger_mock_base(bool log_to_stdout = false) :
        _log_to_stout{log_to_stdout}
    {}

private:
    bool _log_to_stout;

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

        if(_log_to_stout)
        {
            std::cout << source << " " << d.location.to_string() << " " << d.message << "\n";
        }

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
    using diagnostic_logger_mock_base::diagnostic_logger_mock_base;

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
    std::unique_ptr<trompeloeil::expectation> debug_logs_expectation;

    istream_lexer_context(const std::string& input, bool log_to_stdout = false) :
        logger{log_to_stdout},
        filename{"file.cpp"},
        input{input},
        stream{input},
        lexer{stream, logger, filename},
        debug_logs_expectation{
            // Allow debug logs by default
            NAMED_ALLOW_CALL(logger, diagnostic_logged(
                _, // source
                cppast::severity::debug,
                _, // file
                _, // line
                _, // column
                _  // message
            ))
        }
    {
        logger.set_verbose(true);
    }

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

class fuzzy
{
public:
    fuzzy() :
        prng{std::random_device()()}
    {}

    template<typename T>
    T random_int(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
    {
        return std::uniform_int_distribution<T>{min, max}(prng);
    }

    template<typename T>
    T random_float(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
    {
        return std::uniform_real_distribution<T>{min, max}(prng);
    }

    std::size_t random_length(std::size_t max)
    {
        return random_int(static_cast<std::size_t>(1), max);
    }

    struct any
    {
        constexpr bool operator()(char) const
        {
            return true;
        }
    };

    template<typename Predicate>
    char random_char(Predicate predicate = any{})
    {
        char c = random_int<char>();

        while(!predicate(c) || c == '\0')
        {
            c = random_int<char>();
        }

        return c;
    }

    template<typename Predicate>
    void randomize_string(std::string& str, Predicate predicate = any{})
    {
        for(char& c : str)
        {
            c = random_char(predicate);
        }
    }

    template<typename Predicate>
    std::string random_string(Predicate predicate)
    {
        std::string str(random_length(100), ' ');
        randomize_string(str, predicate);
        return str;
    }

    std::string random_string()
    {
        return random_string(any{});
    }

    std::string random_spaces()
    {
        std::string str(random_length(5), ' ');

        for(std::size_t i = 0; i < str.size(); ++i)
        {
            if(random_int<std::size_t>(0, str.size()) == i)
            {
                str[i] = '\n';
            }
        }

        return str;
    }

    std::string random_string_literal()
    {
        return "\"" + random_string([](char c)
        {
            return std::isalnum(c) ||
                c == '+' || c == '-' || c == '*';
        }) + "\"";
    }

    std::string random_integer()
    {
        return std::to_string(random_int<std::int64_t>());
    }

    std::string random_float()
    {
        return std::to_string(random_float<double>());
    }

    std::string random_id()
    {
        auto str = random_string([](char c)
        {
            return std::isalnum(c) || c == '_';
        });

        if(!std::isalpha(str.at(0)))
        {
            str[0] = 'c';
        }

        return str;
    }

    cppast::detail::parser::token random_token()
    {
        using cppast::detail::parser::token;

        auto kind = static_cast<token::token_kind>(
            random_int<char>(static_cast<char>(token::token_kind::end_of_enum), -1)
        );

        token result;
        result.kind = kind;

        switch(kind)
        {
        case token::token_kind::identifier:
            result.token = random_id(); break;
        case token::token_kind::string_literal:
            result.token = random_string_literal(); break;
        case token::token_kind::int_iteral:
            result.token = random_integer(); break;
        case token::token_kind::float_literal:
            result.token = random_float(); break;
        case token::token_kind::double_colon:
            result.token = "::"; break;
        case token::token_kind::comma:
            result.token = ","; break;
        case token::token_kind::bracket_open:
            result.token = "["; break;
        case token::token_kind::bracket_close:
            result.token = "]"; break;
        case token::token_kind::double_bracket_open:
            result.token = "[["; break;
        case token::token_kind::double_bracket_close:
            result.token = "]]"; break;
        case token::token_kind::paren_open:
            result.token = "("; break;
        case token::token_kind::paren_close:
            result.token = ")"; break;
        case token::token_kind::angle_bracket_open:
            result.token = "<"; break;
        case token::token_kind::angle_bracket_close:
            result.token = ">"; break;
        case token::token_kind::unknown:
            result.token = "\"unknown\"";
            result.kind = token::token_kind::string_literal;
            break;
        case token::token_kind::bool_literal:
            result.token = "\"bool\"";
            result.kind = token::token_kind::string_literal;
            break;
        case token::token_kind::unint_literal:
            result.token = "\"uint\"";
            result.kind = token::token_kind::string_literal;
            break;
        default:
            result.token = std::string{{'\"', static_cast<char>(kind), '\"'}};
            result.kind = token::token_kind::string_literal;
            break;
        }

        return result;
    }

    // To generate testable random input, this function generates n random tokens using the above
    // random_token() function, then inserts n + 1 sets of blank input (random spaces/newlines) between
    // the generated tokens, the final input following the pattern "<spaces/newlines>token0<spaces/newlines>token1...<spaces/newlines>tokenN-1<spaces/newlines>"
    // The source location of the generated tokens is then extracted from the leading input before each token (See advance() lambda bellow).

    std::pair<std::string, std::vector<cppast::detail::parser::token>>
    random_tokens(std::size_t count)
    {
        std::size_t line = 0;
        std::int64_t column = -1;
        std::ostringstream input_buffer;
        std::vector<cppast::detail::parser::token> tokens;

        auto advance = [&line, &column](const std::string& input)
        {
            for(std::size_t i = 0; i < input.size(); ++i)
            {
                if(input[i] == '\n')
                {
                    line++;
                    column = -1;
                }
                else
                {
                    column++;
                }
            }
        };

        for(std::size_t i = 0; i < (count * 2 + 1); ++i)
        {
            if(i % 2 == 0)
            {
                // insert spaces between tokens
                auto spaces = random_spaces();
                advance(spaces);
                input_buffer << spaces;
            }
            else
            {
                auto token = random_token();
                token.line = line;
                token.column = column + 1;

                tokens.push_back(token);
                input_buffer << token.token;
                advance(token.token);
            }
        }

        return std::make_pair(
            input_buffer.str(),
            std::move(tokens)
        );
    }

private:
    std::default_random_engine prng;
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

    SECTION("Tokens store the original location in the source")
    {
        istream_lexer_context context{"  12345\n"
            "  12345.0\n"
            "    -12345.0\n"
        };

        context.read_all();
        REQUIRE(context.tokens.size() == 3);
        CHECK(context.tokens[0].line == 0);
        CHECK(context.tokens[0].column == 2);
        CHECK(context.tokens[1].line == 1);
        CHECK(context.tokens[1].column == 2);
        CHECK(context.tokens[2].line == 2);
        CHECK(context.tokens[2].column == 4);
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

            CHECK_FALSE(context.lexer.read_next_token());
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

            CHECK_FALSE(context.lexer.read_next_token());
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
    SECTION("Spaces are ignored")
    {
        istream_lexer_context context{"      \"hello world\"    \"42\"    "};
        CHECK(context.equal_tokens({
            {cppast::detail::parser::token::token_kind::string_literal, "\"hello world\""},
            {cppast::detail::parser::token::token_kind::string_literal, "\"42\""},
        }));
    }

    SECTION("Tokens store the original source location")
    {
        istream_lexer_context context{"  \"hello world\"\n"
            "  \"foobar\"\n"
            "    \"quux\"\n"
        };

        context.read_all();
        REQUIRE(context.tokens.size() == 3);
        CHECK(context.tokens[0].line   == 0);
        CHECK(context.tokens[0].column == 2);
        CHECK(context.tokens[1].line   == 1);
        CHECK(context.tokens[1].column == 2);
        CHECK(context.tokens[2].line   == 2);
        CHECK(context.tokens[2].column == 4);
    }

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

TEST_CASE("istream_lexer reads one char tokens", "[istream_lexer]")
{
    auto test = [](char c, cppast::detail::parser::token::token_kind expected_kind)
    {
        std::string input = "  x  x  \n"
            "  x x x  x\n"
            "x\n"
            "x  x\n";

        for(char& cc : input)
        {
            if(cc == 'x')
            {
                cc = c;
            }
        }

        istream_lexer_context context{input};
        REQUIRE(context.read_all().size() == 9);

        auto check_token = [c, expected_kind](const cppast::detail::parser::token& t, std::size_t line, std::size_t column)
        {
            INFO("token: " << t << ", expected line: " << line << ", expected column: " << column);
            CHECK(t.token == std::string{c});
            CHECK(t.kind == expected_kind);
            CHECK(t.line == line);
            CHECK(t.column == column);
        };

        check_token(context.tokens[0], 0, 2);
        check_token(context.tokens[1], 0, 5);
        check_token(context.tokens[2], 1, 2);
        check_token(context.tokens[3], 1, 4);
        check_token(context.tokens[4], 1, 6);
        check_token(context.tokens[5], 1, 9);
        check_token(context.tokens[6], 2, 0);
        check_token(context.tokens[7], 3, 0);
        check_token(context.tokens[8], 3, 3);
    };

    test('(', cppast::detail::parser::token::token_kind::paren_open);
    test(')', cppast::detail::parser::token::token_kind::paren_close);
    test('[', cppast::detail::parser::token::token_kind::bracket_open);
    test(']', cppast::detail::parser::token::token_kind::bracket_close);
    test('<', cppast::detail::parser::token::token_kind::angle_bracket_open);
    test('>', cppast::detail::parser::token::token_kind::angle_bracket_close);
    test(',', cppast::detail::parser::token::token_kind::comma);
}

TEST_CASE("istream_lexer reads two char tokens", "[istream_lexer]")
{
    auto test = [](char c, cppast::detail::parser::token::token_kind expected_kind)
    {
        std::string input = "  xx  xx  \n"
            "  xx xx xx  xx\n"
            "xx\n"
            "xx  xx\n";

        for(char& cc : input)
        {
            if(cc == 'x')
            {
                cc = c;
            }
        }

        istream_lexer_context context{input};
        REQUIRE(context.read_all().size() == 9);

        auto check_token = [c, expected_kind](const cppast::detail::parser::token& t, std::size_t line, std::size_t column)
        {
            INFO("token: " << t << ", expected line: " << line << ", expected column: " << column);
            CHECK((t.token == std::string{c, c}));
            CHECK(t.kind == expected_kind);
            CHECK(t.line == line);
            CHECK(t.column == column);
        };

        check_token(context.tokens[0], 0, 2);
        check_token(context.tokens[1], 0, 6);
        check_token(context.tokens[2], 1, 2);
        check_token(context.tokens[3], 1, 5);
        check_token(context.tokens[4], 1, 8);
        check_token(context.tokens[5], 1, 12);
        check_token(context.tokens[6], 2, 0);
        check_token(context.tokens[7], 3, 0);
        check_token(context.tokens[8], 3, 4);
    };

    test('[', cppast::detail::parser::token::token_kind::double_bracket_open);
    test(']', cppast::detail::parser::token::token_kind::double_bracket_close);
    test(':', cppast::detail::parser::token::token_kind::double_colon);
}

TEST_CASE("istream_lexer is fuzzed with random input", "[istream_lexer]")
{
    auto fuzz = [](std::size_t count)
    {
        fuzzy fuzzy;

        std::string input;
        std::vector<cppast::detail::parser::token> expected_tokens;

        std::tie(input, expected_tokens) =  fuzzy.random_tokens(count);
        istream_lexer_context context{input};

        INFO("test input: " << input);
        REQUIRE(context.read_all().size() == expected_tokens.size());

        for(std::size_t i = 0; i < expected_tokens.size(); ++i)
        {
            INFO("current token: " << i);
            CHECK(context.tokens[i] == expected_tokens[i]);
        }
    };

    SECTION("Fuzzed with 16 tokens")
    {
        fuzz(16);
    }
    SECTION("Fuzzed with 64 tokens")
    {
        fuzz(32);
    }
    SECTION("Fuzzed with 256 tokens")
    {
        fuzz(256);
    }
    SECTION("Fuzzed with 1024 tokens")
    {
        fuzz(1024);
    }
    SECTION("Fuzzed with 4096 tokens")
    {
        fuzz(4096);
    }
}
