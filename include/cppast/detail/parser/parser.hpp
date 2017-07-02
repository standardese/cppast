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
std::shared_ptr<ast_identifier> parse_identifier(buffered_lexer& lexer);

}

}

}

#endif // CPPAST_DETAIL_PARSER_PARSER_INCLUDED
