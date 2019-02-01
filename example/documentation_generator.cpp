// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

/// \file
/// A primitive documentation generator.
///
/// Given an input file, it will print its documentation including documentation comments.

#include <cppast/code_generator.hpp> // code_generator, generate_code()
#include <cppast/visitor.hpp>        // visit()

#include "example_parser.hpp"

// don't show in code generation
bool is_excluded_synopsis(const cppast::cpp_entity& e, cppast::cpp_access_specifier_kind access)
{
    // exclude privates and those marked for exclusion
    return access == cppast::cpp_private || cppast::has_attribute(e, "documentation::exclude");
}

// don't show in documentation
bool is_excluded_documentation(const cppast::cpp_entity&               e,
                               const cppast::cpp_access_specifier_kind access)
{
    // exclude uninteresting entities
    return e.kind() == cppast::cpp_entity_kind::access_specifier_t
           || e.kind() == cppast::cpp_entity_kind::using_declaration_t
           || e.kind() == cppast::cpp_entity_kind::using_directive_t
           || e.kind() == cppast::cpp_entity_kind::static_assert_t
           || e.kind() == cppast::cpp_entity_kind::include_directive_t
           // and all excluded in synopsis
           || is_excluded_synopsis(e, access);
}

// generates synopsis of an entity
std::string generate_synopsis(const cppast::cpp_entity& e)
{
    // the generator for the synopsis
    class synopsis_generator final : public cppast::code_generator
    {
    public:
        // get the resulting string
        std::string result()
        {
            return std::move(str_);
        }

    private:
        // whether or not the entity is the main entity that is being documented
        bool is_main_entity(const cppast::cpp_entity& e)
        {
            if (cppast::is_templated(e) || cppast::is_friended(e))
                // need to ask the real entity
                return is_main_entity(e.parent().value());
            else
                return &e == &this->main_entity();
        }

        // get some nicer formatting
        cppast::formatting do_get_formatting() const override
        {
            return cppast::formatting_flags::brace_nl | cppast::formatting_flags::comma_ws
                   | cppast::formatting_flags::operator_ws;
        }

        // calculate generation options
        generation_options do_get_options(const cppast::cpp_entity&         e,
                                          cppast::cpp_access_specifier_kind access) override
        {
            if (is_excluded_synopsis(e, access))
                return cppast::code_generator::exclude;
            else if (!is_main_entity(e))
                // only generation declaration for the non-documented entity
                return cppast::code_generator::declaration;
            else
                // default options
                return {};
        }

        // update indendation level
        void do_indent() override
        {
            ++indent_;
        }
        void do_unindent() override
        {
            if (indent_)
                --indent_;
        }

        // write specified tokens
        // need to change indentation for each newline
        void do_write_token_seq(cppast::string_view tokens) override
        {
            if (was_newline_)
            {
                str_ += std::string(indent_ * 2u, ' ');
                was_newline_ = false;
            }

            str_ += tokens.c_str();
        }

        // write + remember newline
        void do_write_newline() override
        {
            str_ += "\n";
            was_newline_ = true;
        }

        std::string str_;
        unsigned    indent_      = 0;
        bool        was_newline_ = false;
    } generator;
    cppast::generate_code(generator, e);
    return generator.result();
}

void generate_documentation(const cppast::cpp_file& file)
{
    // visit each entity
    cppast::visit(file,
                  [](const cppast::cpp_entity& e, cppast::cpp_access_specifier_kind access) {
                      if (is_excluded_documentation(e, access))
                          // exclude this and all children
                          return cppast::visit_filter::exclude_and_children;
                      else if (cppast::is_templated(e) || cppast::is_friended(e))
                          // continue on with children for a dummy entity
                          return cppast::visit_filter::exclude;
                      else
                          return cppast::visit_filter::include;
                  },
                  [](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
                      if (info.is_old_entity())
                          // already done
                          return;

                      // print name
                      std::cout << "## " << cppast::to_string(e.kind()) << " '" << e.name()
                                << "'\n";
                      std::cout << '\n';

                      // print synopsis
                      std::cout << "```\n";
                      std::cout << generate_synopsis(e);
                      std::cout << "```\n\n";

                      // print documentation comment
                      if (e.comment())
                          std::cout << e.comment().value() << '\n';

                      // print separator
                      std::cout << "\n---\n\n";
                  });
    std::cout << "\n\n";
}

int main(int argc, char* argv[])
{
    return example_main(argc, argv, {}, &generate_documentation);
}
