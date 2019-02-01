// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// Generate equality comparisons.
///
/// Given an input file, it will generate comparison operators for each class that has the
/// [[generate::comparison]] attribute.

#include <algorithm>
#include <iostream>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/visitor.hpp>

#include "example_parser.hpp"

// whether or not the token string contains the given token
bool has_token(const cppast::cpp_token_string& str, const char* token)
{
    auto iter = std::find_if(str.begin(), str.end(),
                             [&](const cppast::cpp_token& tok) { return tok.spelling == token; });
    return iter != str.end();
}

// generates equality operator for a class
void generate_op_equal(std::ostream& out, const cppast::cpp_class& c)
{
    out << "inline bool operator==(const " << c.name() << "& lhs, const " << c.name()
        << "& rhs) {\n";
    out << "  return ";

    auto first = true;

    // compare bases
    for (auto& base : c.bases())
    {
        if (cppast::has_attribute(base, "generate::transient"))
            // if they are not marked not to be compared
            continue;

        if (first)
            first = false;
        else
            out << "      && ";
        out << "static_cast<const " << base.name() << "&>(lhs) == static_cast<const " << base.name()
            << "&>(rhs)\n";
    }

    // compare members
    for (auto& member : c)
        if (member.kind() == cppast::cpp_entity_kind::member_variable_t
            && !cppast::has_attribute(member, "generate::transient"))
        {
            // generate comparison code for non-transient member variables
            if (first)
                first = false;
            else
                out << "      && ";
            out << "lhs." << member.name() << " == "
                << "rhs." << member.name() << "\n";
        }

    out << "         ;\n";
    out << "}\n\n";
}

// generate non equality operator for a class
void generate_op_non_equal(std::ostream& out, const cppast::cpp_class& c)
{
    // just forwards
    out << "inline bool operator!=(const " << c.name() << "& lhs, const " << c.name()
        << "& rhs) {\n";
    out << "  return !(lhs == rhs);\n";
    out << "}\n\n";
}

// generate comparison operators for all classes in the file
void generate_comparison(const cppast::cpp_file& file)
{
    cppast::visit(file,
                  [](const cppast::cpp_entity& e) {
                      // only visit non-templated class definitions that have the attribute set
                      return (!cppast::is_templated(e)
                              && e.kind() == cppast::cpp_entity_kind::class_t
                              && cppast::is_definition(e)
                              && cppast::has_attribute(e, "generate::comparison"))
                             // or all namespaces
                             || e.kind() == cppast::cpp_entity_kind::namespace_t;
                  },
                  [](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                      if (e.kind() == cppast::cpp_entity_kind::class_t && !info.is_old_entity())
                      {
                          // it is a new class
                          auto& class_ = static_cast<const cppast::cpp_class&>(e);
                          auto& attribute
                              = cppast::has_attribute(e, "generate::comparison").value();

                          // generate requested operators
                          if (attribute.arguments())
                          {
                              if (has_token(attribute.arguments().value(), "=="))
                                  generate_op_equal(std::cout, class_);
                              if (has_token(attribute.arguments().value(), "!="))
                                  generate_op_non_equal(std::cout, class_);
                          }
                          else
                          {
                              generate_op_equal(std::cout, class_);
                              generate_op_non_equal(std::cout, class_);
                          }
                      }
                      else if (e.kind() == cppast::cpp_entity_kind::namespace_t)
                      {
                          if (info.event == cppast::visitor_info::container_entity_enter)
                              // open namespace
                              std::cout << "namespace " << e.name() << " {\n\n";
                          else // if (info.event == cppast::visitor_info::container_entity_exit)
                              // close namespace
                              std::cout << "}\n";
                      }
                  });
}

int main(int argc, char* argv[])
{
    return example_main(argc, argv, {}, &generate_comparison);
}
