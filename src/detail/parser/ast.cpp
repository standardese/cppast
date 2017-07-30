// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/ast.hpp>

namespace cppast
{

namespace detail
{

namespace parser
{


namespace ast
{

expression_invoke::expression_invoke(const std::shared_ptr<identifier>& callee, const node_list& args) :
    callee{callee},
    args{args}
{
    kind = node_kind::expression_invoke;
}

identifier::identifier(const std::string& name) :
    scope_names{{name}}
{
    kind = node_kind::identifier;
}

identifier::identifier(const std::vector<std::string>& scope_names) :
    scope_names{scope_names}
{
    kind = node_kind::identifier;
}

expression_cpp_attribute::expression_cpp_attribute(const std::shared_ptr<expression_invoke>& body) :
    body{body}
{
    kind = node_kind::expression_cpp_attribute;
}

const std::string& identifier::unqualified_name() const
{
    return scope_names.back();
}

std::string identifier::full_qualified_name() const
{
    std::stringstream ss;

    for(std::size_t i = 0; i < scope_names.size() - 1; ++i)
    {
        ss << scope_names[i] << "::";
    }

    ss << unqualified_name();
    return ss.str();
}

void node::visit(visitor& visitor) const
{
    do_visit(visitor);
}

void node::do_visit(visitor& visitor) const
{
    visitor.on_node(*this);
}

void expression_invoke::do_visit(visitor& visitor) const
{
    visitor.on_node(*this);

    visitor.on_event(visitor::event::children_enter);

    callee->visit(visitor);
    for(const auto& arg : args)
    {
        arg->visit(visitor);
    }

    visitor.on_event(visitor::event::children_exit);
}

void expression_cpp_attribute::do_visit(visitor& visitor) const
{
    visitor.on_node(*this);

    visitor.on_event(visitor::event::children_enter);
    body->visit(visitor);
    visitor.on_event(visitor::event::children_exit);
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

void detailed_visitor::on_node(const node& node)
{
    visit_node(const_cast<class node*>(&node), visit_function{this});
}

std::shared_ptr<node> make_literal_integer(long long value)
{
    return std::make_shared<terminal_number<long long>>(value);
}

std::shared_ptr<node> make_literal_float(double value)
{
    return std::make_shared<terminal_number<double>>(value);
}

std::shared_ptr<node> make_literal_string(const std::string& value)
{
    return std::make_shared<terminal_string>(value);
}

std::shared_ptr<node> make_literal(const token& token)
{
    switch(token.kind)
    {
    case token::token_kind::int_iteral:
        return make_literal_integer(token.int_value());
    case token::token_kind::float_literal:
        return make_literal_float(token.float_value());
    case token::token_kind::string_literal:
        return make_literal_string(token.string_value());
    default:
        return nullptr;
    }
}

} // namespace cppast::detail::parser::ast

} // namespace cppast::detail::parser

} // namespace cppast::detail

} // namespace cppast
