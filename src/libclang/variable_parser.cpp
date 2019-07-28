// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "parse_functions.hpp"

#include <cppast/cpp_member_variable.hpp>
#include <cppast/cpp_variable.hpp>

#include "libclang_visitor.hpp"

using namespace cppast;

std::unique_ptr<cpp_expression> detail::parse_default_value(cpp_attribute_list&          attributes,
                                                            const detail::parse_context& context,
                                                            const CXCursor& cur, const char* name)
{
    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto has_default = false;
    auto got_name    = *name == '\0';
    for (auto paren_count = 0; !stream.done();)
    {
        if (detail::skip_if(stream, "("))
            ++paren_count;
        else if (detail::skip_if(stream, ")"))
            --paren_count;
        else if (!got_name && detail::skip_if(stream, name))
            got_name = true;
        else if (paren_count == 0 && got_name && detail::skip_if(stream, "="))
        {
            // heuristic: we're outside of parens, the name was already encountered
            // and we have an equal sign -> treat this as default value
            // (yes this breaks for evil types)
            has_default = true;
            break;
        }
        else
        {
            auto cur_attributes = detail::parse_attributes(stream, true);
            attributes.insert(attributes.end(), cur_attributes.begin(), cur_attributes.end());
        }
    }
    if (has_default)
        return parse_raw_expression(context, stream, stream.end(),
                                    parse_type(context, cur, clang_getCursorType(cur)));
    else
        return nullptr;
}

std::unique_ptr<cpp_entity> detail::parse_cpp_variable(const detail::parse_context& context,
                                                       const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_VarDecl, detail::assert_handler{});

    auto name          = get_cursor_name(cur);
    auto type          = parse_type(context, cur, clang_getCursorType(cur));
    auto storage_class = get_storage_class(cur);
    auto is_constexpr  = false;

    // just look for thread local or constexpr
    // can't appear anywhere else, so good enough
    detail::cxtokenizer tokenizer(context.tu, context.file, cur);
    for (auto& token : tokenizer)
        if (token.value() == "thread_local")
            storage_class
                = cpp_storage_class_specifiers(storage_class | cpp_storage_class_thread_local);
        else if (token.value() == "constexpr")
            is_constexpr = true;

    cpp_attribute_list attributes;
    auto               default_value = parse_default_value(attributes, context, cur, name.c_str());

    std::unique_ptr<cpp_variable> result;
    if (clang_isCursorDefinition(cur))
    {
        result
            = cpp_variable::build(*context.idx, get_entity_id(cur), name.c_str(), std::move(type),
                                  std::move(default_value), storage_class, is_constexpr);
    }
    else
        result = cpp_variable::build_declaration(get_entity_id(cur), name.c_str(), std::move(type),
                                                 storage_class, is_constexpr);
    context.comments.match(*result, cur);
    result->add_attribute(attributes);
    return result;
}

std::unique_ptr<cpp_entity> detail::parse_cpp_member_variable(const detail::parse_context& context,
                                                              const CXCursor&              cur)
{
    DEBUG_ASSERT(cur.kind == CXCursor_FieldDecl, detail::assert_handler{});

    auto name       = get_cursor_name(cur);
    auto type       = parse_type(context, cur, clang_getCursorType(cur));
    auto is_mutable = clang_CXXField_isMutable(cur) != 0u;

    cpp_attribute_list attributes;
    auto               default_value = parse_default_value(attributes, context, cur, name.c_str());

    std::unique_ptr<cpp_member_variable_base> result;
    if (clang_Cursor_isBitField(cur))
    {
        auto no_bits = clang_getFieldDeclBitWidth(cur);
        DEBUG_ASSERT(no_bits >= 0, detail::parse_error_handler{}, cur, "invalid number of bits");
        if (name.empty())
            result = cpp_bitfield::build(std::move(type), unsigned(no_bits), is_mutable);
        else
            result = cpp_bitfield::build(*context.idx, get_entity_id(cur), name.c_str(),
                                         std::move(type), unsigned(no_bits), is_mutable);
    }
    else
    {
        result = cpp_member_variable::build(*context.idx, get_entity_id(cur), name.c_str(),
                                            std::move(type), std::move(default_value), is_mutable);
    }
    result->add_attribute(attributes);
    context.comments.match(*result, cur);
    return result;
}
