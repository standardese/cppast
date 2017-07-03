#ifndef CPPAST_DETAIL_PARSER_PARSER_INCLUDED
#define CPPAST_DETAIL_PARSER_PARSER_INCLUDED

#include <cppast/detail/parser/ast.hpp>
#include <cppast/detail/parser/buffered_lexer.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

std::shared_ptr<ast_node> parse_expression(buffered_lexer& lexer);
std::shared_ptr<ast_expression_invoke> parse_invoke(buffered_lexer& lexer);
std::pair<bool, node_list> parse_arguments(
    buffered_lexer& lexer,
    const token::token_kind open_delim = token::token_kind::paren_open,
    const token::token_kind close_delim = token::token_kind::paren_close
);
std::shared_ptr<ast_identifier> parse_identifier(buffered_lexer& lexer);
std::shared_ptr<ast_expression_cpp_attribute> parse_cpp_attribute(buffered_lexer& lexer);

}

}

}

#endif // CPPAST_DETAIL_PARSER_PARSER_INCLUDED
