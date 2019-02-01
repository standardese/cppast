// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// Generates enum category functions.
///
/// Given an input file, it will generate definitions for functions marked with
/// [[generate::enum_category(name)]]. The function takes an enumerator and will return true if it
/// is marked with the same category.

#include <algorithm>
#include <cassert>
#include <iostream>

#include <cppast/cpp_enum.hpp>     // cpp_enum
#include <cppast/cpp_function.hpp> // cpp_function
#include <cppast/visitor.hpp>      // visit()

#include "example_parser.hpp"

// returns whether or not the given enumerator has the given category
bool is_category(const cppast::cpp_enum_value& e, const std::string& name)
{
    if (auto attr = cppast::has_attribute(e, "generate::enum_category"))
    {
        // ... by looking for the token
        auto iter
            = std::find_if(attr.value().arguments().value().begin(),
                           attr.value().arguments().value().end(),
                           [&](const cppast::cpp_token& tok) { return tok.spelling == name; });
        return iter != attr.value().arguments().value().end();
    }
    else
        return false;
}

// returns the enum the parameter type refers to
const cppast::cpp_enum& get_enum(const cppast::cpp_entity_index&       index,
                                 const cppast::cpp_function_parameter& param)
{
    auto& param_type = param.type();
    // it is an enum
    assert(param_type.kind() == cppast::cpp_type_kind::user_defined_t);
    // lookup definition
    auto& definition = static_cast<const cppast::cpp_user_defined_type&>(param_type)
                           .entity()
                           .get(index)[0u]
                           .get();

    assert(definition.kind() == cppast::cpp_entity_kind::enum_t);
    return static_cast<const cppast::cpp_enum&>(definition);
}

// generates the function definitions
void generate_enum_category(const cppast::cpp_entity_index& index, const cppast::cpp_file& file)
{
    cppast::visit(file,
                  [](const cppast::cpp_entity& e) {
                      // only visit function declarations that have the attribute set
                      return (e.kind() == cppast::cpp_entity_kind::function_t
                              && !cppast::is_definition(e)
                              && cppast::has_attribute(e, "generate::enum_category"))
                             // or all namespaces
                             || e.kind() == cppast::cpp_entity_kind::namespace_t;
                  },
                  [&](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                      if (e.kind() == cppast::cpp_entity_kind::function_t)
                      {
                          // a new function, generate implementation
                          assert(info.is_new_entity());

                          auto category = cppast::has_attribute(e, "generate::enum_category")
                                              .value()
                                              .arguments()
                                              .value()
                                              .as_string();

                          auto& func = static_cast<const cppast::cpp_function&>(e);
                          // return type must be bool
                          assert(func.return_type().kind() == cppast::cpp_type_kind::builtin_t
                                 && static_cast<const cppast::cpp_builtin_type&>(func.return_type())
                                            .builtin_type_kind()
                                        == cppast::cpp_bool);

                          // single parameter...
                          assert(std::next(func.parameters().begin()) == func.parameters().end());
                          auto& param = *func.parameters().begin();
                          auto& enum_ = get_enum(index, param);

                          // generate function definition
                          std::cout << "inline bool " << func.name() << "("
                                    << cppast::to_string(param.type()) << " e) {\n";

                          // generate switch
                          std::cout << "  switch (e) {\n";
                          for (const auto& enumerator : enum_)
                          {
                              std::cout << "  case " << enum_.name() << "::" << enumerator.name()
                                        << ":\n";
                              if (is_category(enumerator, category))
                                  std::cout << "    return true;\n";
                              else
                                  std::cout << "    return false;\n";
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
    cppast::cpp_entity_index index;
    return example_main(argc, argv, index,
                        [&](const cppast::cpp_file& file) { generate_enum_category(index, file); });
}
