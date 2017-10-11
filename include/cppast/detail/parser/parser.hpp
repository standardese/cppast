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

/// Implements a parser of C-like invoke expressions
///
/// The parser class implements an LL(1) parser for a mini C-like
/// language as follows:
///
/// ```
///   Expr := Terminal | InvokeExpr | (Expr)
///   InvokeExpr := Identifier | Identifier InvokeArgs
///   Identifier := identifierToken | Identifier::identifierToken
///   InvokeArgs := () | (Expr) | (Expr, Expr, ...)
///   Terminal   := stringLiteralToken | integerLiteralToken | boolLiteralToken | floatLiteralToken
/// ```
///
/// That simple grammar is implemented with simple invoke-like
/// expressions in mind, but the language can be easily modified.
/// Each grammar rule is implemented as a `do_parse_xxxx()` function
/// that is overridable to extend the language or customize it.
///
/// Grammar expansions are returned as shared pointers to the corresponding
/// AST nodes (See `cppast/detail/parser/ast.hpp`).
///
/// The parser also implements a non-compliant wip C++ attribute parser.
/// Non-compliant because the parser defines C++ attributes as invoke
/// expressions surrounded by `[[ ]]`, i.e.:
///
/// ```
/// CppAttrExpr := [[InvokeExpr]]
/// ```
///
/// Note that parsers can only be used once.
class parser
{
public:
    /// Initializes a parser with a given lexer
    /// \notes Any lexer is valid, the parser will internally build
    /// its own lookahead lexer on top of it.
    parser(lexer& lexer);
    virtual ~parser() = default;

    /// Tries to parse a C++ attribute.
    /// \returns the AST representing the parsed attribute, `nullptr` if
    /// an error occurred parsing the input.
    std::shared_ptr<ast::expression_cpp_attribute> parse_cpp_attribute();

    /// Tries to parse an invoke expression.
    /// \returns the AST representing the parsed invoke expression, `nullptr` if
    /// an error occurred parsing the input.
    std::shared_ptr<ast::expression_invoke> parse_invoke();

    /// Returns the logger used to return parsing diagnostics.
    /// \notes The parser shares the logger used by the lexer
    const diagnostic_logger& logger() const;

    /// Checks whether the parser is ready to parse input
    bool good() const;

protected:
    /// Tries to follow the Expr (expression) production
    /// \returns If an expression was parsed successfully, returns
    /// a pointer to the AST node representing the expression.
    /// nullptr if parsing fails.
    /// \notes The default implementation of this production
    /// includes parsing (ignoring) of enclosing expression parents,
    /// terminals, and recursion into parsing invoke expressions.
    /// While this is the outermost production in the grammar, it is
    /// not directly exposed since the most usual use case is to
    /// specifically try to parse invoke expressions or attributes, not
    /// generic input.
    virtual std::shared_ptr<ast::node> do_parse_expression();

    /// Tries to follow the InvokeExpr production
    /// \returns a pointer to an `cppast::detail::parser::ast::expression_invoke`
    /// AST node representing the parsed invoke expression. nullptr if parsing
    /// fails.
    virtual std::shared_ptr<ast::expression_invoke> do_parse_invoke();

    /// Tries to follow the InvokeArgs production
    /// \returns A pair `(ok, sequence)` where:
    ///  - `ok`: Returns true or false to indicate if the arguments were
    ///  successfully parsed.
    ///  - `sequence`: A vector of AST nodes representing the parsed arguments.
    /// \notes While InvokeExpr production uses parens by default, `do_parse_arguments()`
    /// implements a generic comma-separated sequence parser, with the sequence delimiters
    /// are specified through the `open_delimp  and `close_delim` parameters of the
    /// function.
    /// This could be useful to parse other productions in an extended language, such as
    /// an array ('[', ']') or an initializer list (`{', '}').
    virtual std::pair<bool, ast::node_list> do_parse_arguments(
        const token::token_kind open_delim = token::token_kind::paren_open,
        const token::token_kind close_delim = token::token_kind::paren_close
    );

    /// Tries to parse an identifier following the Identifier production
    /// \returns A `cppast::detail::parser::ast::identifier` node representing
    /// the identifier. nullptr on error.
    virtual std::shared_ptr<ast::identifier> do_parse_identifier();

    /// Tries to parse a C++ attribute
    /// \returns A `cppast::detail::parser::ast::expression_cpp_attribute` representing
    /// the attribute. nullptr on error.
    /// \notes A C++ attribute is parsed as an invoke expression surrounded by double
    /// square brackets. This is not compliant to the C++ Standard (There are potential
    /// ill-formed attributes that this parser will recognize as valid) but most simple
    /// cases are successfully parsed:
    ///
    /// ``` cpp
    /// [[noreturn]]
    /// [[foo::bar]]
    /// [[deprecated("foobar")]]
    /// ```
    virtual std::shared_ptr<ast::expression_cpp_attribute> do_parse_cpp_attribute();

private:
    buffered_lexer _lexer;
};

}

}

}

#endif // CPPAST_DETAIL_PARSER_PARSER_HPP_INCLUDED
