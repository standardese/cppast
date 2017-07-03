#include <cppast/detail/parser/ast.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{

ast_expression_invoke::ast_expression_invoke(const std::shared_ptr<ast_identifier>& callee, const node_list& args) :
    callee{callee},
    args{args}
{
    kind = node_kind::expression_invoke;
}

ast_identifier::ast_identifier(const std::string& name) :
    scope_names{{name}}
{
    kind = node_kind::identifier;
}

ast_identifier::ast_identifier(const std::vector<std::string>& scope_names) :
    scope_names{scope_names}
{
    kind = node_kind::identifier;
}

ast_expression_cpp_attribute::ast_expression_cpp_attribute(const std::shared_ptr<ast_expression_invoke>& body) :
    body{body}
{
    kind = node_kind::expression_cpp_attribute;
}

const std::string& ast_identifier::unqualified_name() const
{
    return scope_names.back();
}

std::string ast_identifier::full_qualified_name() const
{
    std::stringstream ss;

    for(std::size_t i = 0; i < scope_names.size() - 1; ++i)
    {
        ss << scope_names[i] << "::";
    }

    ss << unqualified_name();
    return ss.str();
}

void ast_node::visit(ast_visitor& visitor) const
{
    do_visit(visitor);
}

void ast_node::do_visit(ast_visitor& visitor) const
{
    visitor.on_node(*this);
}

void ast_expression_invoke::do_visit(ast_visitor& visitor) const
{
    visitor.on_node(*this);

    visitor.on_event(ast_visitor::event::children_enter);

    callee->visit(visitor);
    for(const auto& arg : args)
    {
        arg->visit(visitor);
    }

    visitor.on_event(ast_visitor::event::children_exit);
}

void ast_expression_cpp_attribute::do_visit(ast_visitor& visitor) const
{
    visitor.on_node(*this);

    visitor.on_event(ast_visitor::event::children_enter);
    body->visit(visitor);
    visitor.on_event(ast_visitor::event::children_exit);
}

// C++14, we miss you
struct visit_function
{
    detailed_visitor* self;

    template<typename T>
    void operator()(T* node)
    {
        self->on_node(*node);
    }
};

void detailed_visitor::on_node(const ast_node& node)
{
    visit_node(const_cast<ast_node*>(&node), visit_function{this});
}

std::shared_ptr<ast_node> literal_integer(long long value)
{
    return std::make_shared<ast_terminal_number<long long>>(value);
}

std::shared_ptr<ast_node> literal_float(double value)
{
    return std::make_shared<ast_terminal_number<double>>(value);
}

std::shared_ptr<ast_node> literal_string(const std::string& value)
{
    return std::make_shared<ast_terminal_string>(value);
}

std::shared_ptr<ast_node> literal(const token& token)
{
    switch(token.kind)
    {
    case token::token_kind::int_iteral:
        return literal_integer(token.int_value());
    case token::token_kind::float_literal:
        return literal_float(token.float_value());
    case token::token_kind::string_literal:
        return literal_string(token.string_value());
    default:
        return nullptr;
    }
}

}

}

}
