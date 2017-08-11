#ifndef CPPAST_TEST_DETAIL_PARSER_TOKEN_GENERATOR_HPP_INCLUDED
#define CPPAST_TEST_DETAIL_PARSER_TOKEN_GENERATOR_HPP_INCLUDED

#include <cppast/detail/parser/token.hpp>
#include <random>

namespace cppast
{

namespace test
{

class token_generator
{
public:
    token_generator() :
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

    bool random_bool()
    {
        return random_int(0, 1);
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

    std::string random_boolean()
    {
        if(random_bool())
        {
            return "true";
        }
        else
        {
            return "false";
        }
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

    cppast::detail::parser::token random_token(cppast::detail::parser::token::token_kind kind)
    {
        using cppast::detail::parser::token;

        token result;
        result.kind = kind;

        switch(kind)
        {
        case token::token_kind::identifier:
            result.token = random_id(); break;
        case token::token_kind::string_literal:
            result.token = random_string_literal(); break;
        case token::token_kind::int_literal:
            result.token = random_integer(); break;
        case token::token_kind::float_literal:
            result.token = random_float(); break;
        case token::token_kind::bool_literal:
            result.token = random_boolean(); break;
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
        default:
            result.token = std::string{{'\"', static_cast<char>(kind), '\"'}};
            result.kind = token::token_kind::string_literal;
            break;
        }

        return result;
    }

    cppast::detail::parser::token random_token()
    {
        using cppast::detail::parser::token;

        auto kind = static_cast<token::token_kind>(
            random_int<char>(static_cast<char>(token::token_kind::end_of_enum), -1)
        );

        return random_token(kind);
    }

    std::vector<cppast::detail::parser::token> random_tokens(const std::vector<cppast::detail::parser::token::token_kind>& kinds)
    {
        std::vector<cppast::detail::parser::token> tokens;
        tokens.reserve(kinds.size());

        for(auto kind : kinds)
        {
            tokens.push_back(random_token(kind));
        }

        return tokens;
    }

private:
    std::default_random_engine prng;
};

} // namespace cppast::test

} // namespace cppast

#endif // CPPAST_TEST_DETAIL_PARSER_TOKEN_GENERATOR_HPP_INCLUDED
