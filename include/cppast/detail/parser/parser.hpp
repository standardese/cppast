// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_PARSER_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_PARSER_HPP_INCLUDED

#include <cppast/detail/parser/ast.hpp>
#include <cppast/detail/parser/buffered_lexer.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

class parser
{
public:
    parser(lexer& lexer);
    virtual ~parser() = default;

    std::shared_ptr<ast_expression_cpp_attribute> parse_cpp_attribute();
    std::shared_ptr<ast_expression_invoke> parse_invoke();

    const diagnostic_logger& logger() const;
    bool good() const;

protected:
    virtual std::shared_ptr<ast_node> do_parse_expression();
    virtual std::shared_ptr<ast_expression_invoke> do_parse_invoke();
    virtual std::pair<bool, node_list> do_parse_arguments(
        const token::token_kind open_delim = token::token_kind::paren_open,
        const token::token_kind close_delim = token::token_kind::paren_close
    );
    virtual std::shared_ptr<ast_identifier> do_parse_identifier();
    virtual std::shared_ptr<ast_expression_cpp_attribute> do_parse_cpp_attribute();

private:
    buffered_lexer _lexer;
};

}

}

}

#endif // CPPAST_DETAIL_PARSER_PARSER_HPP_INCLUDED
