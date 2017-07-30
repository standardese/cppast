// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED

#include <cppast/detail/parser/lexer.hpp>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>


namespace cppast
{

namespace detail
{

namespace parser
{

namespace ast
{

class node;

class visitor
{
public:
    enum class event
    {
        children_enter,
        children_exit
    };

    virtual ~visitor() = default;

    virtual void on_node(const node& node) {}

    virtual void on_event(event event) {}
};

class node
{
public:
    enum class node_kind
    {
        unespecified,
        terminal_float,
        terminal_integer,
        terminal_string,
        identifier,
        expression_invoke,
        expression_cpp_attribute
    };

    node_kind kind = node_kind::unespecified;

    virtual ~node() = default;

    void visit(visitor& visitor) const;

protected:
    virtual void do_visit(visitor& visitor) const;
};

using node_list = std::vector<std::shared_ptr<node>>;

template<typename T>
struct terminal : public node
{
    terminal(const T& value) :
        value{value}
    {
        if(std::is_same<T, std::string>::value)
        {
            kind = node_kind::terminal_string;
        }
        else if(std::is_floating_point<T>::value)
        {
            kind = node_kind::terminal_float;
        }
        else if(std::is_integral<T>::value)
        {
            kind = node_kind::terminal_integer;
        }
    }

    T value;
};

template<typename T>
struct terminal_number : public terminal<T>
{
    using terminal<T>::terminal;

    static_assert(std::is_arithmetic<T>::value, "An arithmetic type is required for numeric terminals");

    static constexpr node::node_kind node_class_kind = (std::is_floating_point<T>::value ?
            node::node_kind::terminal_float :
            node::node_kind::terminal_integer);
};

using terminal_integer = terminal_number<long long>;
using terminal_float   = terminal_number<double>;

struct terminal_string : public terminal<std::string>
{
    using terminal<std::string>::terminal;

    static constexpr node::node_kind node_class_kind = node::node_kind::terminal_string;
};

struct identifier : public node
{
    identifier(const std::string& name);
    identifier(const std::vector<std::string>& scope_names);

    std::vector<std::string> scope_names;

    const std::string& unqualified_name() const;
    std::string full_qualified_name() const;

    static constexpr node::node_kind node_class_kind = node::node_kind::identifier;
};

struct expression_invoke : public node
{
    expression_invoke(const std::shared_ptr<identifier>& callee, const node_list& args);

    std::shared_ptr<identifier> callee;
    node_list args;

    static constexpr node_kind node_class_kind = node_kind::expression_invoke;

private:
    void do_visit(visitor& visitor) const override final;
};

struct expression_cpp_attribute : public node
{
    expression_cpp_attribute(const std::shared_ptr<expression_invoke>& body);

    std::shared_ptr<expression_invoke> body;

    static constexpr node_kind node_class_kind = node_kind::expression_cpp_attribute;

private:
    void do_visit(visitor& visitor) const override final;
};

template<typename T>
T* node_cast(node* node)
{
    if(T::node_class_kind == node->kind)
    {
        return static_cast<T*>(node);
    }
}

template<typename T>
std::shared_ptr<T> node_cast(const std::shared_ptr<node>& node)
{
    if(T::node_class_kind == node->kind)
    {
        return {node, static_cast<T*>(node)};
    }
}

template<typename Function>
void visit_node(node* node, Function function)
{
    switch(node->kind)
    {
    case node::node_kind::terminal_integer:
        function(node_cast<terminal_integer>(node)); break;
    case node::node_kind::terminal_float:
        function(node_cast<terminal_float>(node)); break;
    case node::node_kind::terminal_string:
        function(node_cast<terminal_string>(node)); break;
    case node::node_kind::identifier:
        function(node_cast<identifier>(node)); break;
    case node::node_kind::expression_invoke:
        function(node_cast<expression_invoke>(node)); break;
    case node::node_kind::expression_cpp_attribute:
        function(node_cast<expression_cpp_attribute>(node)); break;
    default:
        throw std::logic_error{"unespecified kind nodes cannot be visited"};
    }
}

template<typename Function>
void visit_node(const std::shared_ptr<node>& node, Function function)
{
    visit_node(node.get(), function);
}

class detailed_visitor : public visitor
{
public:
    virtual void on_node(const terminal_integer& node) {}
    virtual void on_node(const terminal_float& node) {}
    virtual void on_node(const terminal_string& node) {}
    virtual void on_node(const identifier& node) {}
    virtual void on_node(const expression_invoke& node) {}
    virtual void on_node(const expression_cpp_attribute& node) {}

private:
    void on_node(const node& node) override final;
};

std::shared_ptr<node> make_literal_integer(long long value);
std::shared_ptr<node> make_literal_float(float value);
std::shared_ptr<node> make_literal_string(const std::string& str);
std::shared_ptr<node> make_literal(const token& token);

} // namespace cppast::detail::parser::ast

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED
