// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cassert>
#include <iostream>

#include <cppast/cpp_array_type.hpp>
#include <cppast/cpp_class.hpp>
#include <cppast/cpp_entity_kind.hpp>
#include <cppast/cpp_enum.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_function_type.hpp>
#include <cppast/cpp_function.hpp>
#include <cppast/cpp_namespace.hpp>
#include <cppast/cpp_variable.hpp>
#include <cppast/cpp_alias_template.hpp>
#include <cppast/visitor.hpp>

std::unique_ptr<cppast::cpp_file> make_file(const cppast::cpp_entity_index& idx)
{
    using namespace cppast;

    cpp_file::builder file_builder("main.cpp");
    file_builder.add_child(cpp_namespace::builder("foo", false).finish(idx, "foo"_id));

    cpp_namespace::builder ns_builder("bar", false);
    ns_builder.add_child(cpp_namespace::builder("foo", false).finish(idx, "bar::foo"_id));
    file_builder.add_child(ns_builder.finish(idx, "bar"_id));

    cpp_enum::builder e_builder("e", true);
    e_builder.add_value(cpp_enum_value::build(idx, "e::a"_id, "a", nullptr));
    file_builder.add_child(e_builder.finish(idx, "e"_id));

    cpp_function::builder f_builder("func", cpp_builtin_type::build("int"));
    f_builder.add_parameter(
        cpp_function_parameter::build(idx, "param"_id, "param", cpp_builtin_type::build("int")));
    f_builder.is_variadic();
    file_builder.add_child(f_builder.finish(idx, "func"_id));

    cpp_class::builder c_builder("type", cpp_class_kind::class_t, false);
    c_builder.access_specifier(cpp_public);
    c_builder.add_child(cpp_variable::build(idx, "var"_id, "var", cpp_builtin_type::build("int"),
                                            nullptr, cpp_storage_class_none, false));
    file_builder.add_child(c_builder.finish(idx, "type"_id));

    cpp_alias_template::builder a_builder(
        cpp_type_alias::build("alias", cpp_template_parameter_type::build(
                                           cpp_template_type_parameter_ref("T"_id, "T"))));
    a_builder.add_parameter(
        cpp_template_type_parameter::build(idx, "T"_id, "T", cpp_template_keyword::keyword_typename,
                                           false, nullptr));
    file_builder.add_child(a_builder.finish(idx, "alias"_id));

    return file_builder.finish(idx);
}

int main()
{
    cppast::cpp_entity_index idx;
    auto                     file = make_file(idx);

    auto level = 0;
    cppast::visit(*file, [&](const cppast::cpp_entity& e, cppast::visitor_info info) {
        if (info == cppast::visitor_info::container_entity_exit)
            --level;
        else
        {
            std::cout << std::string(level, ' ') << cppast::to_string(e.kind()) << ' ' << e.name()
                      << ' ' << cppast::full_name(e) << '\n';
            if (info == cppast::visitor_info::container_entity_enter)
                ++level;
        }

        return true;
    });
}
