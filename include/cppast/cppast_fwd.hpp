// Copyright (C) 2021 Julian Rüth <julian.rueth@fsfe.org>
// SPDX-License-Identifier: MIT

#ifndef CPPAST_FORWARD_HPP_INCLUDED
#define CPPAST_FORWARD_HPP_INCLUDED

namespace cppast
{

class code_generator;
class compile_config;
class cpp_access_specifier;
class cpp_alias_template;
class cpp_array_type;
class cpp_attribute;
class cpp_auto_type;
class cpp_base_class;
class cpp_bitfield;
class cpp_builtin_type;
class cpp_class;
class cpp_class_template;
class cpp_class_template_specialization;
class cpp_concept;
class cpp_constructor;
class cpp_conversion_op;
class cpp_cv_qualified_type;
class cpp_decltype_auto_type;
class cpp_decltype_type;
class cpp_dependent_type;
class cpp_destructor;
class cpp_entity;
class cpp_entity_index;
class cpp_enum;
class cpp_enum_value;
class cpp_expression;
class cpp_file;
class cpp_forward_declarable;
class cpp_friend;
class cpp_function;
class cpp_function_base;
class cpp_function_parameter;
class cpp_function_template;
class cpp_function_template_specialization;
class cpp_function_type;
class cpp_include_directive;
class cpp_language_linkage;
class cpp_literal_expression;
class cpp_macro_definition;
class cpp_macro_parameter;
class cpp_member_function;
class cpp_member_function_base;
class cpp_member_function_type;
class cpp_member_object_type;
class cpp_member_variable_base;
class cpp_namespace;
class cpp_non_type_template_parameter;
class cpp_pointer_type;
class cpp_reference_type;
class cpp_scope_name;
class cpp_static_assert;
class cpp_template;
class cpp_template_argument;
class cpp_template_instantiation_type;
class cpp_template_parameter;
class cpp_template_parameter_type;
class cpp_template_specialization;
class cpp_template_template_parameter;
class cpp_template_type_parameter;
class cpp_token_string;
class cpp_type;
class cpp_type_alias;
class cpp_unexposed_entity;
class cpp_unexposed_expression;
class cpp_unexposed_type;
class cpp_user_defined_type;
class cpp_using_declaration;
class cpp_using_directive;
class cpp_variable;
class cpp_variable_base;
class cpp_variable_template;
class diagnostic_logger;
class libclang_compilation_database;
class libclang_compile_config;
class libclang_error;
class libclang_parser;
class parser;
class string_view;

enum class compile_flag;
enum class cpp_attribute_kind;
enum class cpp_class_kind;
enum class cpp_entity_kind;
enum class cpp_expression_kind;
enum class cpp_include_kind;
enum class cpp_standard;
enum class cpp_template_keyword;
enum class cpp_token_kind;
enum class cpp_type_kind;
enum class cpp_virtual_flags;
enum class formatting_flags;
enum class severity;
enum class visit_filter;

enum cpp_access_specifier_kind : int;
enum cpp_builtin_type_kind : int;
enum cpp_cv : int;
enum cpp_function_body_kind : int;
enum cpp_reference : int;
enum cpp_storage_class_specifiers : int;
enum visitor_result : bool;

struct cpp_doc_comment;
struct cpp_entity_id;
struct cpp_token;
struct diagnostic;
struct newl_t;
struct source_location;
struct visitor_info;
struct whitespace_t;

template <class Derived, typename T>
class cpp_entity_container;
template <class Parser>
class simple_file_parser;
template <typename T, typename Predicate>
class basic_cpp_entity_ref;

} // namespace cppast

#endif // CPPAST_FORWARD_HPP_INCLUDED
