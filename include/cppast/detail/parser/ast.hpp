#ifndef CPPAST_DETAIL_PARSER_AST_INCLUDED
#define CPPAST_DETAIL_PARSER_AST_INCLUDED

#include <cppast/detail/parser/lexer.hpp>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>


namespace cppast
{

namespace detail
{

namespace parser
{

class ast_node;

class ast_visitor
{
public:
    enum class event
    {
        children_enter,
        children_exit
    };

    virtual ~ast_visitor() = default;

    virtual void on_node(const ast_node& node) {}

    virtual void on_event(event event) {}
};

class ast_node
{
public:
    enum class node_kind
    {
        unespecified,
        terminal_float,
        terminal_integer,
        terminal_string,
        identifier,
        expression_invoke
    };

    node_kind kind = node_kind::unespecified;

    virtual ~ast_node() = default;

    void visit(ast_visitor& visitor) const;

protected:
    virtual void do_visit(ast_visitor& visitor) const;
};

using node_list = std::vector<std::shared_ptr<ast_node>>;

template<typename T>
struct ast_terminal : public ast_node
{
    ast_terminal(const T& value) :
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
struct ast_terminal_number : public ast_terminal<T>
{
    using ast_terminal<T>::ast_terminal;

    static_assert(std::is_arithmetic<T>::value, "An arithmetic type is required for numeric terminals");

    static constexpr ast_node::node_kind node_class_kind = (std::is_floating_point<T>::value ?
            ast_node::node_kind::terminal_float :
            ast_node::node_kind::terminal_integer);
};

using ast_terminal_integer = ast_terminal_number<long long>;
using ast_terminal_float   = ast_terminal_number<double>;

struct ast_terminal_string : public ast_terminal<std::string>
{
    using ast_terminal<std::string>::ast_terminal;

    static constexpr ast_node::node_kind node_class_kind = ast_node::node_kind::terminal_string;
};

struct ast_identifier : public ast_node
{
    ast_identifier(const std::string& name);
    ast_identifier(const std::vector<std::string>& scope_names);

    std::vector<std::string> scope_names;

    const std::string& unqualified_name() const;
    std::string full_qualified_name() const;

    static constexpr ast_node::node_kind node_class_kind = ast_node::node_kind::identifier;
};

struct ast_expression_invoke : public ast_node
{
    ast_expression_invoke(const std::shared_ptr<ast_identifier>& callee, const node_list& args);

    std::shared_ptr<ast_identifier> callee;
    node_list args;

    static constexpr node_kind node_class_kind = node_kind::expression_invoke;

private:
    void do_visit(ast_visitor& visitor) const override final;
};

template<typename T>
T* node_cast(ast_node* node)
{
    if(T::node_class_kind == node->kind)
    {
        return static_cast<T*>(node);
    }
}

template<typename T>
std::shared_ptr<T> node_cast(const std::shared_ptr<ast_node>& node)
{
    if(T::node_class_kind == node->kind)
    {
        return {node, static_cast<T*>(node)};
    }
}

template<typename Function>
void visit_node(ast_node* node, Function function)
{
    switch(node->kind)
    {
    case ast_node::node_kind::terminal_integer:
        function(node_cast<ast_terminal_integer>(node)); break;
    case ast_node::node_kind::terminal_float:
        function(node_cast<ast_terminal_float>(node)); break;
    case ast_node::node_kind::terminal_string:
        function(node_cast<ast_terminal_string>(node)); break;
    case ast_node::node_kind::identifier:
        function(node_cast<ast_identifier>(node)); break;
    case ast_node::node_kind::expression_invoke:
        function(node_cast<ast_expression_invoke>(node)); break;
    default:
        throw std::logic_error{"unespecified kind nodes cannot be visited"};
    }
}

template<typename Function>
void visit_node(const std::shared_ptr<ast_node>& node, Function function)
{
    visit_node(node.get(), function);
}

class detailed_visitor : public ast_visitor
{
public:
    virtual void on_node(const ast_terminal_integer& node) {}
    virtual void on_node(const ast_terminal_float& node) {}
    virtual void on_node(const ast_terminal_string& node) {}
    virtual void on_node(const ast_identifier& node) {}
    virtual void on_node(const ast_expression_invoke& node) {}

private:
    void on_node(const ast_node& node) override final;
};

std::shared_ptr<ast_node> literal_integer(long long value);
std::shared_ptr<ast_node> literal_float(float value);
std::shared_ptr<ast_node> literal_string(const std::string& str);
std::shared_ptr<ast_node> literal(const token& token);

}

}

}

#endif // CPPAST_DETAIL_PARSER_AST_INCLUDED
