#include "cppast2json.h"

namespace cppast
{
void to_json(json& j, const cpp_class_kind& p)
{
    j = json{
        {"name", cppast::to_string(p)},
        {"value", (int)p},
    };
}

void to_json(json& j, const cpp_entity_kind& p)
{
    j = json{
        {"name", cppast::to_string(p)},
        {"value", (int)p},
    };
}

void to_json(json& j, const cpp_include_kind& p)
{
    j = json{
        {"name", cppast::to_string(p)},
        {"value", (int)p},
    };
}

void to_json(json& j, const cpp_token_kind& p)
{
    j = json{
        {"name", cppast::to_string(p)},
        {"value", (int)p},
    };
}

void to_json(json& j, const cpp_type_kind& p)
{
    j = json{
        {"name", cppast::to_string(p)},
        {"value", (int)p},
    };
}

void to_json(json& j, const cpp_scope_name& p)
{
    j = json{
        {"name", p.name()},
        {"is_templated", p.is_templated()},
    };
}

void to_json(json& j, const cpp_attribute& p)
{
    j = json{
        {"name", p.name()},           {"kind", p.kind()}, {"is_variadic", p.is_variadic()},
        {"arguments", p.arguments()}, {"scope", nullptr},
    };
    if (p.scope().has_value())
    {
        j["scope"] = p.scope().value();
    }
}

void to_json(json& j, const cpp_forward_declarable& p)
{
    j = json{
        {"is_definition", p.is_definition()},   {"is_declaration", p.is_declaration()},
        {"definition", p.definition()},         {"semantic_parent", p.semantic_parent()},
        {"semantic_scope", p.semantic_scope()},
    };
}

void to_json(json& j, const cpp_entity_id& p)
{
    j = json({
        {"value", p.value_},
    });
}

void to_json(json& j, const cpp_type& p)
{
    j = json({
        {"kind", p.kind()},
        {"name", cppast::to_string(p)},
    });
}

void to_json(json& j, const cpp_expression& p)
{
    j = json({
        {"kind", p.kind()},
        {"type", p.type()},
    });
}

void to_json(json& j, const cpp_doc_comment& p)
{
    j = json({
        {"content", p.content},
        {"line", p.line},
    });
}

void to_json(json& j, const cpp_variable_base& p)
{
    j = json({
        {"type", p.type()},
        {"default_value", p.default_value()},
    });
}

void to_json(json& j, const cpp_member_variable_base& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_variable_base&>(p));
    j["is_mutable"] = p.is_mutable();
}

void to_json(json& j, const cpp_function_base& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_forward_declarable&>(p));
    j["parameters"]         = p.parameters();
    j["body_kind"]          = p.body_kind();
    j["noexcept_condition"] = p.noexcept_condition();
    j["is_variadic"]        = p.is_variadic();
    j["signature"]          = p.signature();
}

void to_json(json& j, const cpp_member_function_base& p)
{
    j                  = static_cast<const cpp_function_base&>(p);
    j["return_type"]   = p.return_type();
    j["is_virtual"]    = p.is_virtual();
    j["virtual_info"]  = p.virtual_info();
    j["cv_qualifier"]  = p.cv_qualifier();
    j["ref_qualifier"] = p.ref_qualifier();
    j["is_constexpr"]  = p.is_constexpr();
    j["is_consteval"]  = p.is_consteval();
}

void to_json(json& j, const cpp_token_string& p)
{
    j = p.as_string();
}

void to_json(json& j, const cpp_entity& p)
{
    j = json{
        {"kind", p.kind()},     {"name", p.name()},   {"scope_name", p.scope_name()},
        {"parent", p.parent()}, {"comment", nullptr}, {"attributes", p.attributes()},
    };
    if (p.comment().has_value())
    {
        j["comment"] = p.comment().value();
    }
}

void to_json_x(json& j, const cpp_entity& p)
{
    switch (p.kind())
    {
#define CPPAST_DETAIL_HANDLE(Name)                                                                 \
    case cpp_entity_kind::Name##_t:                                                                \
        j = static_cast<const cpp_##Name&>(p);                                                     \
        break;

        CPPAST_DETAIL_HANDLE(file)

        CPPAST_DETAIL_HANDLE(macro_parameter)
        CPPAST_DETAIL_HANDLE(macro_definition)
        CPPAST_DETAIL_HANDLE(include_directive)

        CPPAST_DETAIL_HANDLE(language_linkage)
        CPPAST_DETAIL_HANDLE(namespace)
        CPPAST_DETAIL_HANDLE(namespace_alias)
        CPPAST_DETAIL_HANDLE(using_directive)
        CPPAST_DETAIL_HANDLE(using_declaration)

        CPPAST_DETAIL_HANDLE(type_alias)

        CPPAST_DETAIL_HANDLE(enum)
        CPPAST_DETAIL_HANDLE(enum_value)

        CPPAST_DETAIL_HANDLE(class)
        CPPAST_DETAIL_HANDLE(access_specifier)
        CPPAST_DETAIL_HANDLE(base_class)

        CPPAST_DETAIL_HANDLE(variable)
        CPPAST_DETAIL_HANDLE(member_variable)
        CPPAST_DETAIL_HANDLE(bitfield)

        CPPAST_DETAIL_HANDLE(function_parameter)
        CPPAST_DETAIL_HANDLE(function)
        CPPAST_DETAIL_HANDLE(member_function)
        CPPAST_DETAIL_HANDLE(conversion_op)
        CPPAST_DETAIL_HANDLE(constructor)
        CPPAST_DETAIL_HANDLE(destructor)

        CPPAST_DETAIL_HANDLE(friend)

        CPPAST_DETAIL_HANDLE(template_type_parameter)
        CPPAST_DETAIL_HANDLE(non_type_template_parameter)
        CPPAST_DETAIL_HANDLE(template_template_parameter)

        CPPAST_DETAIL_HANDLE(alias_template)
        CPPAST_DETAIL_HANDLE(variable_template)
        CPPAST_DETAIL_HANDLE(function_template)
        CPPAST_DETAIL_HANDLE(function_template_specialization)
        CPPAST_DETAIL_HANDLE(class_template)
        CPPAST_DETAIL_HANDLE(class_template_specialization)
        CPPAST_DETAIL_HANDLE(concept)

        CPPAST_DETAIL_HANDLE(static_assert)
#undef CPPAST_DETAIL_HANDLE
    }
}

void to_json(json& j, const cpp_file& p)
{
    j                       = static_cast<const cpp_entity&>(p);
    j["unmatched_comments"] = p.unmatched_comments();
}

void to_json(json& j, const cpp_macro_parameter& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_macro_definition& p)
{
    j                     = static_cast<const cpp_entity&>(p);
    j["replacement"]      = p.replacement();
    j["is_object_like"]   = p.is_object_like();
    j["is_function_like"] = p.is_function_like();
    j["is_variadic"]      = p.is_variadic();
    j["parameters"]       = p.parameters();
}

void to_json(json& j, const cpp_include_directive& p)
{
    j                 = static_cast<const cpp_entity&>(p);
    j["target"]       = p.target();
    j["include_kind"] = p.include_kind();
    j["full_path"]    = p.full_path();
}

void to_json(json& j, const cpp_language_linkage& p)
{
    j             = static_cast<const cpp_entity&>(p);
    j["is_block"] = p.is_block();
}

void to_json(json& j, const cpp_namespace& p)
{
    j                 = static_cast<const cpp_entity&>(p);
    j["is_inline"]    = p.is_inline();
    j["is_nested"]    = p.is_nested();
    j["is_anonymous"] = p.is_anonymous();
}

void to_json(json& j, const cpp_namespace_alias& p)
{
    j           = static_cast<const cpp_entity&>(p);
    j["target"] = p.target();
}

void to_json(json& j, const cpp_using_directive& p)
{
    j           = static_cast<const cpp_entity&>(p);
    j["target"] = p.target();
}

void to_json(json& j, const cpp_using_declaration& p)
{
    j           = static_cast<const cpp_entity&>(p);
    j["target"] = p.target();
}

void to_json(json& j, const cpp_type_alias& p)
{
    j                    = static_cast<const cpp_entity&>(p);
    j["underlying_type"] = p.underlying_type();
}

void to_json(json& j, const cpp_enum& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_forward_declarable&>(p));
    j["underlying_type"]   = p.underlying_type();
    j["has_explicit_type"] = p.has_explicit_type();
    j["is_scoped"]         = p.is_scoped();
    /*j["children"]          = json::array();
    for (auto& it : p)
    {
        j["children"].push_back(static_cast<const cpp_enum_value&>(it));
    }*/
}

void to_json(json& j, const cpp_enum_value& p)
{
    j          = static_cast<const cpp_entity&>(p);
    j["value"] = p.value();
}

void to_json(json& j, const cpp_class& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_forward_declarable&>(p));
    j["class_kind"] = p.class_kind();
    j["is_final"]   = p.is_final();
    j["bases"]      = p.bases();
}

void to_json(json& j, const cpp_access_specifier& p)
{
    j                     = static_cast<const cpp_entity&>(p);
    j["access_specifier"] = p.access_specifier();
}

void to_json(json& j, const cpp_base_class& p)
{
    j                     = static_cast<const cpp_entity&>(p);
    j["type"]             = p.type();
    j["access_specifier"] = p.access_specifier();
    j["is_virtual"]       = p.is_virtual();
}

void to_json(json& j, const cpp_variable& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_variable_base&>(p));
    j.merge_patch(static_cast<const cpp_forward_declarable&>(p));
    j["storage_class"] = p.storage_class();
    j["is_constexpr"]  = p.is_constexpr();
}

void to_json(json& j, const cpp_member_variable& p)
{
    j = static_cast<const cpp_member_variable_base&>(p);
}

void to_json(json& j, const cpp_bitfield& p)
{
    j            = static_cast<const cpp_member_variable_base&>(p);
    j["no_bits"] = p.no_bits();
}

void to_json(json& j, const cpp_function_parameter& p)
{
    j = static_cast<const cpp_entity&>(p);
    j.merge_patch(static_cast<const cpp_variable_base&>(p));
}

void to_json(json& j, const cpp_function& p)
{
    j                  = static_cast<const cpp_function_base&>(p);
    j["return_type"]   = p.return_type();
    j["storage_class"] = p.storage_class();
    j["is_constexpr"]  = p.is_constexpr();
    j["is_consteval"]  = p.is_consteval();
}

void to_json(json& j, const cpp_member_function& p)
{
    j = static_cast<const cpp_member_function_base&>(p);
}

void to_json(json& j, const cpp_conversion_op& p)
{
    j                = static_cast<const cpp_member_function_base&>(p);
    j["is_explicit"] = p.is_explicit();
}

void to_json(json& j, const cpp_constructor& p)
{
    j                 = static_cast<const cpp_function_base&>(p);
    j["is_explicit"]  = p.is_explicit();
    j["is_constexpr"] = p.is_constexpr();
    j["is_consteval"] = p.is_consteval();
}

void to_json(json& j, const cpp_destructor& p)
{
    j                 = static_cast<const cpp_function_base&>(p);
    j["is_virtual"]   = p.is_virtual();
    j["virtual_info"] = p.virtual_info();
}

void to_json(json& j, const cpp_friend& p)
{
    j           = static_cast<const cpp_entity&>(p);
    j["entity"] = p.entity();
    j["type"]   = p.type();
}

void to_json(json& j, const cpp_template_type_parameter& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_non_type_template_parameter& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_template_template_parameter& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_alias_template& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_variable_template& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_function_template& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_function_template_specialization& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_class_template& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_class_template_specialization& p)
{
    j = static_cast<const cpp_entity&>(p);
}

void to_json(json& j, const cpp_concept& p)
{
    j                          = static_cast<const cpp_entity&>(p);
    j["parameters"]            = p.parameters();
    j["constraint_expression"] = p.constraint_expression();
}

void to_json(json& j, const cpp_static_assert& p)
{
    j               = static_cast<const cpp_entity&>(p);
    j["expression"] = p.expression();
    j["message"]    = p.message();
}
} // namespace cppast
