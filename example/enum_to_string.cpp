// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// Generates enum `to_string()` code.
///
/// Given an input file, it will generate a to_string() function for all enums marked with
/// [[generate::to_string]].

#include <iostream>

#include <cppast/cpp_enum.hpp> // cpp_enum
#include <cppast/visitor.hpp>  // visit()

#include "example_parser.hpp"

void generate_to_string(const cppast::cpp_file& file)
{
    cppast::visit(file,
                  [](const cppast::cpp_entity& e) {
                      // only visit enum definitions that have the attribute set
                      return (e.kind() == cppast::cpp_entity_kind::enum_t
                              && cppast::is_definition(e)
                              && cppast::has_attribute(e, "generate::to_string"))
                             // or all namespaces
                             || e.kind() == cppast::cpp_entity_kind::namespace_t;
                  },
                  [](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                      if (e.kind() == cppast::cpp_entity_kind::enum_t && !info.is_old_entity())
                      {
                          // a new enum, generate to string function
                          auto& enum_ = static_cast<const cppast::cpp_enum&>(e);

                          // write function header
                          std::cout << "inline const char* to_string(const " << enum_.name()
                                    << "& e) {\n";

                          // generate switch
                          std::cout << "  switch (e) {\n";
                          for (const auto& enumerator : enum_)
                          {
                              std::cout << "  case " << enum_.name() << "::" << enumerator.name()
                                        << ":\n";

                              // attribute can be used to override the string
                              if (auto attr
                                  = cppast::has_attribute(enumerator, "generate::to_string"))
                                  std::cout << "    return "
                                            << attr.value().arguments().value().as_string()
                                            << ";\n";
                              else
                                  std::cout << "    return \"" << enumerator.name() << "\";\n";
                          }
                          std::cout << "  }\n";

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
    return example_main(argc, argv, {}, &generate_to_string);
}
