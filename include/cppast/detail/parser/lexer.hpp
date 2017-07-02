#ifndef CPPAST_DETAIL_PARSER_LEXER_INCLUDED
#define CPPAST_DETAIL_PARSER_LEXER_INCLUDED

#include <string>
#include <istream>
#include <sstream>
#include <type_safe/optional.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

struct token
{
    enum class token_kind : char
    {
        identifier           = -1,
        string_literal       = -2,
        int_iteral           = -3,
        unint_literal        = -4,
        float_literal        = -5,
        bool_literal         = -6,
        double_colon         = -7,
        comma                = -8,
        semicolon            = -9,
        bracket_open         = -10,
        bracket_close        = -11,
        double_bracket_open  = -12,
        double_bracket_close = -13,
        paren_open           = -14,
        paren_close          = -15,
        angle_bracket_open   = -16,
        angle_bracket_close  = -17
    };

    token_kind kind;
    std::string token;
    std::size_t line;
    std::size_t column;

    std::size_t length() const;

    const std::string& string_value() const;
    long long int_value() const;
    unsigned long long uint_value() const;
    double float_value() const;
};

std::ostream& operator<<(std::ostream& os, const token::token_kind kind);
std::ostream& operator<<(std::ostream& os, const token& token);

class lexer
{
public:
    lexer(std::istream& input);

    std::size_t current_pos() const;

    virtual bool read_next_token();
    virtual const token& current_token() const;

    struct error_info
    {
        std::string message;
        std::size_t begin;
        std::size_t end;
    };

    bool good() const;
    bool eof() const;
    const type_safe::optional<error_info>& error() const;

private:
    std::istream& _input;
    std::ostringstream _token_buffer;
    token _current_token;
    std::size_t _pos;
    char _last_char;
    type_safe::optional<error_info> _error;

    void save_token(const token::token_kind kind, const std::string& token = "");

    friend struct error_builder;
    struct error_builder
    {
        error_builder(lexer* self, std::size_t begin, std::size_t end);

        template<typename T>
        error_builder& operator<<(const T& value)
        {
            stream << value;
            return *this;
        }

        ~error_builder();

        lexer* self;
        std::ostringstream stream;
        std::size_t begin;
        std::size_t end;
    };

    error_builder set_error(std::size_t begin, std::size_t end);
    error_builder set_error(std::size_t begin);
};

}

}

}

#endif // CPPAST_DETAIL_PARSER_LEXER_INCLUDED
