// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED
#define CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED

#include <cppast/detail/parser/lexer.hpp>
#include <cppast/detail/utils/overloaded_function.hpp>
#include <cppast/detail/assert.hpp>
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

/// Provides a visitation interface for the parser AST
class visitor
{
public:
    /// Represents AST traversal events
    enum class event
    {
        children_enter, /// The visitor started traversing children of a node
        children_exit   /// The visitor finished traversiong children of a node
    };

    virtual ~visitor() = default;

    /// Invoked when an AST node is reached
    /// \param node The node currently being visited
    virtual void on_node(const node& /* node */) {}

    /// Fired when a traversal event occurs
    virtual void on_event(event /* event */) {}
};

template<typename T>
const T& node_cast(const node& node);
template<typename T>
T& node_cast(node& node);

/// Represents a node in the parser AST
class node
{
public:
    /// Represents the different types of nodes supported by the parser AST
    enum class node_kind
    {
        unespecified,
        terminal_float,
        terminal_integer,
        terminal_string,
        terminal_boolean,
        identifier,
        expression_invoke,
        expression_cpp_attribute
    };

    /// The type of this node.
    /// \notes Any AST node is tagged with this kind value to know which node
    /// represents even in type erased context such as when accessing the node
    /// through its base class. See [`as()`]() and [`node_cast()`]() for safe node conversions.
    node_kind kind = node_kind::unespecified;

    virtual ~node() = default;

    /// Starts AST traversal from the current node using the given
    /// AST visitor.
    void visit(visitor& visitor) const;


    /// Returns this node instance converted to the given AST node type.
    /// \returns A reference to this converted to the requested type if this node
    /// if of the kind represented by the given AST node type. Throws [`ast::bad_node_cast`]()
    /// otherwise.
    template<typename Node>
    Node& as()
    {
        return node_cast<Node>(*this);
    }

    /// Returns this node instance converted to the given AST node type.
    /// \returns A reference to this converted to the requested type if this node
    /// if of the kind represented by the given AST node type. Throws [`ast::bad_node_cast`]()
    /// otherwise.
    template<typename Node>
    const Node& as() const
    {
        return node_cast<Node>(*this);
    }

protected:
    /// AST visitation implementation, to be customized by AST node types.
    virtual void do_visit(visitor& visitor) const;
};

/// Returns a human readable string representation of an AST node kind.
constexpr const char* to_string(node::node_kind kind)
{
    switch(kind)
    {
    case node::node_kind::unespecified:
        return "unespecified";
    case node::node_kind::terminal_float:
        return "terminal_float";
    case node::node_kind::terminal_integer:
        return "terminal_integer";
    case node::node_kind::terminal_string:
        return "terminal_string";
    case node::node_kind::terminal_boolean:
        return "terminal_boolean";
    case node::node_kind::identifier:
        return "identifier";
    case node::node_kind::expression_invoke:
        return "expression_invoke";
    case node::node_kind::expression_cpp_attribute:
        return "expression_cpp_attribute";
    }

    return nullptr;
}

std::ostream& operator<<(std::ostream& os, node::node_kind kind);

/// Converts a pointer to an AST node to a pointer of the given AST node type.
/// \returns A pointer of the requested type pointing to the same node object if
/// the node has the same kind that the AST node type given represents. Returns
/// nullptr otherwise.
template<typename T>
T* node_cast(node* node)
{
    if(T::node_class_kind == node->kind)
    {
        return static_cast<T*>(node);
    }
    else
    {
        return nullptr;
    }
}

/// Indicates a failure in the conversion of an AST node.
struct bad_node_cast : public std::exception
{
    using std::exception::exception;
};

/// Converts a reference to an AST node to a reference of the given AST node type.
/// \returns A reference of the requested type pointing to the same node object if
/// the node has the kind represented by that AST node type. Else throws [`ast::bad_node_cast`]().
template<typename T>
const T& node_cast(const node& node)
{
    auto* ptr = node_cast<T>(&node);

    if(ptr != nullptr)
    {
        return *ptr;
    }
    else
    {
        throw ast::bad_node_cast{};
    }
}

/// Converts a reference to an AST node to a reference of the given AST node type.
/// \returns A reference of the requested type pointing to the same node object if
/// the node has the kind represented by that AST node type. Else throws [`ast::bad_node_cast`]().
template<typename T>
T& node_cast(node& node)
{
    auto* ptr = node_cast<T>(&node);

    if(ptr != nullptr)
    {
        return *ptr;
    }
    else
    {
        throw ast::bad_node_cast{};
    }
}

/// Converts a shared pointer  to an AST node to a shared pointer of the given AST node type.
/// \returns A shared pointer of the requested type pointing to the same node object if
/// the node has the kind represented by that AST node type. Else return a null shared pointer
/// of the requested type.
template<typename T>
std::shared_ptr<T> node_cast(const std::shared_ptr<node>& node)
{
    return {node, node_cast<T>(node.get())};
}

/// A sequence of AST ndoes.
using node_list = std::vector<std::shared_ptr<node>>;

/// Represents an AST terminal.
/// \tparam T The value type of the terminal.
template<typename T>
struct terminal : public node
{
    /// Initializes a terminal by its value and (optionally) the token where that value
    /// was extracted from.
    terminal(
        const T& value,
        const type_safe::optional<cppast::detail::parser::token>& token = type_safe::nullopt) :
        value{value},
        token{token}
    {
        if(std::is_same<T, std::string>::value)
        {
            kind = node_kind::terminal_string;
        }
        else if(std::is_same<T, bool>::value)
        {
            kind = node_kind::terminal_boolean;
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

    /// Value of the terminal.
    T value;

    /// The token from which this terminal node was generated.
    type_safe::optional<cppast::detail::parser::token> token;
};

/// Represents a numeric terminal node.
/// \tparam T The value type of the terminal.
template<typename T>
struct terminal_number : public terminal<T>
{
    using terminal<T>::terminal;

    static_assert(std::is_arithmetic<T>::value, "An arithmetic type is required for numeric terminals");

    static constexpr node::node_kind node_class_kind = (std::is_floating_point<T>::value ?
            node::node_kind::terminal_float :
            node::node_kind::terminal_integer);
};

/// An integer terminal node.
using terminal_integer = terminal_number<long long>;

/// A floating point terminal node.
using terminal_float   = terminal_number<double>;

/// An string terminal node.
struct terminal_string : public terminal<std::string>
{
    using terminal<std::string>::terminal;

    static constexpr node::node_kind node_class_kind = node::node_kind::terminal_string;
};

/// A boolean terminal node.
struct terminal_boolean : public terminal<bool>
{
    using terminal<bool>::terminal;

    static constexpr node::node_kind node_class_kind = node::node_kind::terminal_boolean;
};

/// An identifier AST node.
struct identifier : public node
{
    /// Initializes a non-qualified terminal with its non-qualified name.
    /// \notes Terminals initialized this way are assumed to be defined in
    /// the global namespace (i.e. "foo" is non-qualified "::foo").
    identifier(const std::string& name);

    /// Initializes an identifier with its full qualified name, represented
    /// as the sequence of the scope names that form it. For example,
    /// `["foo", "bar", "quux"]` is "::foo::bar::quux".
    identifier(const std::vector<std::string>& scope_names);

    /// Returns the sequence of scope names that form the full qualified name
    /// of the identifier.
    std::vector<std::string> scope_names;

    /// Returns the unqualified name of the identifier, i.e. "foo" for "std::foo".
    const std::string& unqualified_name() const;

    /// Returns the full qualified name of the identifier, i.e. "foo::bar::quux".
    /// \notes Note that the returned name is not completelly full-qualified, the
    /// initial double colon referencing the global namespace is omitted.
    std::string full_qualified_name() const;

    static constexpr node::node_kind node_class_kind = node::node_kind::identifier;
};

/// Represents an invoke expression
struct expression_invoke : public node
{
    /// Initializes an invoke expression with the callee identifier and the set
    /// of call arguments.
    expression_invoke(const std::shared_ptr<identifier>& callee, const node_list& args);

    /// Callee identifier node.
    std::shared_ptr<identifier> callee;

    /// Set of invoke argument nodes.
    node_list args;

    static constexpr node_kind node_class_kind = node_kind::expression_invoke;

private:
    void do_visit(visitor& visitor) const override final;
};

/// Represents a C++ attribute expression
struct expression_cpp_attribute : public node
{
    /// Initializes an attribute with its body expression.
    expression_cpp_attribute(const std::shared_ptr<expression_invoke>& body);

    /// The attribute body node.
    std::shared_ptr<expression_invoke> body;

    static constexpr node_kind node_class_kind = node_kind::expression_cpp_attribute;

private:
    void do_visit(visitor& visitor) const override final;
};

namespace
{

// Reflection would help here, isn't?
template<typename Function>
void do_visit_node(node* node, Function function)
{
    switch(node->kind)
    {
    case node::node_kind::terminal_integer:
        function(node_cast<terminal_integer>(node)); break;
    case node::node_kind::terminal_float:
        function(node_cast<terminal_float>(node)); break;
    case node::node_kind::terminal_string:
        function(node_cast<terminal_string>(node)); break;
    case node::node_kind::terminal_boolean:
        function(node_cast<terminal_boolean>(node)); break;
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

}

/// Invokes the given callback with a pointer of the dynamic type
/// of the given AST node.
/// \notes Given a callback callable with all the supported AST node type,
/// this function performs the conversion of the node to its dynamic type and
/// calls the appropiate callback overload. For the function to work, the callback
/// **must** implement an overload for each AST node type.
template<typename Function>
void visit_node(node* node, Function function)
{
    do_visit_node(node, function);
}

/// Invokes the given callback with a pointer of the dynamic type
/// of the given AST node.
/// \notes Given a callback callable with all the supported AST node type,
/// this function performs the conversion of the node to its dynamic type and
/// calls the appropiate callback overload. For the function to work, the callback
/// **must** implement an overload for each AST node type.
template<typename Function>
void visit_node(const node* node, Function function)
{
    // yeah....
    visit_node(const_cast<ast::node*>(node), [function](auto* node)
    {
        function(const_cast<const decltype(node)>(node));
    });
}

/// Invokes the given callback with a pointer of the dynamic type
/// of the given AST node.
/// \notes Given a callback callable with all the supported AST node type,
/// this function performs the conversion of the node to its dynamic type and
/// calls the appropiate callback overload. For the function to work, the callback
/// **must** implement an overload for each AST node type.
template<typename Function>
void visit_node(const std::shared_ptr<node>& node, Function function)
{
    visit_node(node.get(), function);
}

/// Invokes the given callback with a reference of the dynamic type
/// of the given AST node.
/// \notes Given a callback callable with all the supported AST node type,
/// this function performs the conversion of the node to its dynamic type and
/// calls the appropiate callback overload. For the function to work, the callback
/// **must** implement an overload for each AST node type.
template<typename Function>
void visit_node(node& node, Function function)
{
    visit_node(&node, [function](auto* node)
    {
        function(*node);
    });
}

/// Invokes the given callback with a reference of the dynamic type
/// of the given AST node.
/// \notes Given a callback callable with all the supported AST node type,
/// this function performs the conversion of the node to its dynamic type and
/// calls the appropiate callback overload. For the function to work, the callback
/// **must** implement an overload for each AST node type.
template<typename Function>
void visit_node(const node& node, Function function)
{
    visit_node(&node, [function](auto* node)
    {
        function(*node);
    });
}

/// Provides an AST visitor with a callback per AST node type
class detailed_visitor : public visitor
{
public:
    /// Invoked when the current node is an integer terminal.
    virtual void on_node(const terminal_integer& /* node */) {}

    /// Invoked when the current node is a floating point terminal.
    virtual void on_node(const terminal_float&   /* node */) {}

    /// Invoked when the current node is a string terminal.
    virtual void on_node(const terminal_string&  /* node */) {}

    /// Invoked when the current node is an bool terminal.
    virtual void on_node(const terminal_boolean& /* node */) {}

    /// Invoked when the current node is an identifier.
    virtual void on_node(const identifier&        /* node */) {}

    /// Invoked when the current node is an invoke expression.
    virtual void on_node(const expression_invoke& /* node */) {}

    /// Invoked when the current node is a C++ attribute expression.
    virtual void on_node(const expression_cpp_attribute& /* node */) {}

private:
    void on_node(const node& node) override final;
};

/// Creates an integer literal with the given value.
std::shared_ptr<node> make_literal_integer(long long value);

/// Creates a floating point literal with the given value.
std::shared_ptr<node> make_literal_float(float value);

/// Creates a string literal with the given value.
std::shared_ptr<node> make_literal_string(const std::string& str);

/// Creates a boolean literal with the given value.
std::shared_ptr<node> make_literal_boolean(bool value);

/// Creates a literal with the given value.
std::shared_ptr<node> make_literal(const token& token);

} // namespace cppast::detail::parser::ast

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_PARSER_AST_HPP_INCLUDED
