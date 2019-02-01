// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// Serialization code generation.
///
/// Given an input file, it will generate a serialize() function for each class marked with
/// [[generate::serialize]].

#include <iostream>

#include <cppast/cpp_class.hpp>
#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_type.hpp>
#include <cppast/visitor.hpp>

#include "example_parser.hpp"

// whether or not a type is a C string, i.e. char pointer
bool is_c_string(const cppast::cpp_type& type)
{
    if (type.kind() != cppast::cpp_type_kind::pointer_t)
        return false;

    // get the pointee
    auto& pointee = cppast::remove_cv(static_cast<const cppast::cpp_pointer_type&>(type).pointee());
    if (pointee.kind() != cppast::cpp_type_kind::builtin_t)
        return false;

    // check the builtin type kind
    auto builtin = static_cast<const cppast::cpp_builtin_type&>(pointee).builtin_type_kind();
    return builtin == cppast::cpp_char || builtin == cppast::cpp_char16
           || builtin == cppast::cpp_char32 || builtin == cppast::cpp_wchar;
}

// generate a serialization call for a member
void generate_serialize_member(std::ostream& out, const cppast::cpp_member_variable& member)
{
    auto& type = cppast::remove_cv(member.type());

    if (cppast::has_attribute(member, "generate::transient"))
        // don't serialize transient members
        return;
    else if (auto attr = cppast::has_attribute(member, "generate::serialize"))
    {
        // generate code as specified by the attributes
        out << "  " << attr.value().arguments().value().as_string() << ";\n";
    }
    else if (type.kind() == cppast::cpp_type_kind::builtin_t)
    {
        // generate hypothetical member function call for builtin types
        out << "  s.serialize(obj." << member.name() << ");\n";
    }
    else if (type.kind() == cppast::cpp_type_kind::user_defined_t)
    {
        // generate ADL call
        out << "  serialize(s, obj." << member.name() << ");\n";
    }
    else if (is_c_string(type))
    {
        // generate another hypothetical member function call
        out << "  s.serialize_string(obj." << member.name() << ");\n";
    }
    else
        throw std::invalid_argument("cannot serialize member " + member.name());
}

// generate serialization function
void generate_serialize(const cppast::cpp_file& file)
{
    cppast::visit(file,
                  [](const cppast::cpp_entity& e) {
                      // only visit non-templated class definitions that have the attribute set
                      return (!cppast::is_templated(e)
                              && e.kind() == cppast::cpp_entity_kind::class_t
                              && cppast::is_definition(e)
                              && cppast::has_attribute(e, "generate::serialize"))
                             // or all namespaces
                             || e.kind() == cppast::cpp_entity_kind::namespace_t;
                  },
                  [](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                      if (e.kind() == cppast::cpp_entity_kind::class_t && !info.is_old_entity())
                      {
                          auto& class_ = static_cast<const cppast::cpp_class&>(e);

                          std::cout << "inline void serialize(const foo::serializer& s, const "
                                    << class_.name() << "& obj) {\n";

                          // serialize base classes
                          for (auto& base : class_.bases())
                              if (!cppast::has_attribute(base, "generate::transient"))
                                  std::cout << " serialize(s, static_cast<const " << base.name()
                                            << "&>(obj));\n";

                          // serialize member variables
                          for (auto& member : class_)
                          {
                              if (member.kind() == cppast::cpp_entity_kind::member_variable_t)
                                  generate_serialize_member(std::cout,
                                                            static_cast<
                                                                const cppast::cpp_member_variable&>(
                                                                member));
                          }

                          std::cout << "}\n\n";
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
    return example_main(argc, argv, {}, generate_serialize);
}
