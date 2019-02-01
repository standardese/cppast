// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_function.hpp>
#include <cppast/cpp_member_function.hpp>

#include "libclang_visitor.hpp"
#include "parse_functions.hpp"

using namespace cppast;

namespace
{
std::unique_ptr<cpp_function_parameter> parse_parameter(const detail::parse_context& context,
                                                        const CXCursor&              cur)
{
    auto name = detail::get_cursor_name(cur);
    auto type = detail::parse_type(context, cur, clang_getCursorType(cur));

    cpp_attribute_list attributes;
    auto default_value = detail::parse_default_value(attributes, context, cur, name.c_str());

    std::unique_ptr<cpp_function_parameter> result;
    if (name.empty())
        result = cpp_function_parameter::build(std::move(type), std::move(default_value));
    else
        result
            = cpp_function_parameter::build(*context.idx, detail::get_entity_id(cur), name.c_str(),
                                            std::move(type), std::move(default_value));
    result->add_attribute(attributes);
    return result;
}

template <class Builder>
void add_parameters(const detail::parse_context& context, Builder& builder, const CXCursor& cur)
{
    if (clang_getCursorKind(cur) == CXCursor_FunctionTemplate)
    {
        // clang_Cursor_getNumArguments() doesn't work here
        // (of course it doesn't...)
        detail::visit_children(cur, [&](const CXCursor& child) {
            if (clang_getCursorKind(child) != CXCursor_ParmDecl)
                return;

            try
            {
                auto parameter = parse_parameter(context, child);
                builder.add_parameter(std::move(parameter));
            }
            catch (detail::parse_error& ex)
            {
                context.error = true;
                context.logger->log("libclang parser", ex.get_diagnostic(context.file));
            }
            catch (std::logic_error& ex)
            {
                context.error = true;
                context.logger->log("libclang parser",
                                    diagnostic{ex.what(),
                                               detail::make_location(context.file, child),
                                               severity::error});
            }
        });
    }
    else
    {
        auto no = clang_Cursor_getNumArguments(cur);
        DEBUG_ASSERT(no != -1, detail::parse_error_handler{}, cur,
                     "unexpected number of arguments");
        for (auto i = 0; i != no; ++i)
            try
            {
                auto parameter
                    = parse_parameter(context, clang_Cursor_getArgument(cur, unsigned(i)));
                builder.add_parameter(std::move(parameter));
            }
            catch (detail::parse_error& ex)
            {
                context.error = true;
                context.logger->log("libclang parser", ex.get_diagnostic(context.file));
            }
            catch (std::logic_error& ex)
            {
                context.error = true;
                context.logger
                    ->log("libclang parser",
                          diagnostic{ex.what(),
                                     detail::make_location(context.file,
                                                           clang_Cursor_getArgument(cur,
                                                                                    unsigned(i))),
                                     severity::error});
            }
    }
}

bool is_templated_cursor(const CXCursor& cur)
{
    return clang_getTemplateCursorKind(cur) != CXCursor_NoDeclFound
           || !clang_Cursor_isNull(clang_getSpecializedCursorTemplate(cur));
}

// precondition: after the name
void skip_parameters(detail::cxtoken_stream& stream)
{
    if (stream.peek() == "<")
        // specialization arguments
        detail::skip_brackets(stream);
    detail::skip_brackets(stream);
}

std::vector<CXCursor> get_semantic_parents(CXCursor cur)
{
    std::vector<CXCursor> result;
    for (; !clang_isTranslationUnit(clang_getCursorKind(cur));
         cur = clang_getCursorSemanticParent(cur))
        result.push_back(cur);
    return result;
}

bool is_class(const CXCursor& parent)
{
    auto kind = clang_getCursorKind(parent);
    return kind == CXCursor_ClassDecl || kind == CXCursor_StructDecl || kind == CXCursor_UnionDecl
           || kind == CXCursor_ClassTemplate || kind == CXCursor_ClassTemplatePartialSpecialization;
}

// returns the scope where the function is contained in
// for regular functions that is the lexcial parent
// for friend functions it is the enclosing scope of the class
CXCursor get_definition_scope(const CXCursor& cur, bool is_friend)
{
    auto parent = clang_getCursorLexicalParent(cur);
    if (is_friend)
    {
        // find the lexical parent that isn't a class
        // as the definition scope is a namespace
        while (is_class(parent))
            parent = clang_getCursorSemanticParent(parent);

        DEBUG_ASSERT(clang_getCursorKind(parent) == CXCursor_Namespace
                         || clang_getCursorKind(parent) == CXCursor_TranslationUnit,
                     detail::parse_error_handler{}, cur,
                     "unable to find definition scope of friend");
    }
    return parent;
}

bool equivalent_cursor(const CXCursor& a, const CXCursor& b)
{
    if (clang_getCursorKind(a) == clang_getCursorKind(b)
        && clang_getCursorKind(a) == CXCursor_Namespace)
        return detail::cxstring(clang_getCursorUSR(a)) == detail::cxstring(clang_getCursorUSR(b));
    else
        return clang_equalCursors(a, b) == 1;
}

type_safe::optional<cpp_entity_ref> parse_scope(const CXCursor& cur, bool is_friend)
{
    std::string scope_name;

    auto friended = clang_getCursorReferenced(cur);
    if (is_friend && !clang_Cursor_isNull(friended))
    {
        // it refers to another function
        // find the common parent between the two cursors
        // scope is the scope from the common parent down to the function

        auto friended_parents = get_semantic_parents(friended);
        auto cur_parents      = get_semantic_parents(get_definition_scope(cur, true));

        // remove common parents
        while (!friended_parents.empty() && !cur_parents.empty()
               && equivalent_cursor(friended_parents.back(), cur_parents.back()))
        {
            friended_parents.pop_back();
            cur_parents.pop_back();
        }
        DEBUG_ASSERT(!clang_isTranslationUnit(clang_getCursorKind(friended_parents.back()))
                         && !friended_parents.empty(),
                     detail::parse_error_handler{}, cur,
                     "invalid common parent of friend and friended");

        // scope consists of all remaining parents of friended
        // (last one is cursor itself)
        for (auto iter = friended_parents.rbegin(); iter != std::prev(friended_parents.rend());
             ++iter)
        {
            auto parent_name = detail::cxstring(clang_getCursorDisplayName(*iter));
            scope_name += parent_name.std_str() + "::";
        }
    }
    else
    {
        // find the difference between the definition scope parent and semantic parent
        // all semantic parents in between form the scope
        // the definition scope is the lexical parent for regular functions,
        // and the scope outside of the class for friend functions
        for (auto definition                                = get_definition_scope(cur, is_friend),
                  parent                                    = clang_getCursorSemanticParent(cur);
             !equivalent_cursor(definition, parent); parent = clang_getCursorSemanticParent(parent))
        {
            DEBUG_ASSERT(!clang_isTranslationUnit(clang_getCursorKind(parent)),
                         detail::parse_error_handler{}, cur,
                         "infinite loop while calculating scope");
            auto parent_name = detail::cxstring(clang_getCursorDisplayName(parent));
            scope_name       = parent_name.std_str() + "::" + std::move(scope_name);
        }
    }

    if (scope_name.empty())
        return type_safe::nullopt;
    else
        return cpp_entity_ref(detail::get_entity_id(clang_getCursorSemanticParent(cur)),
                              std::move(scope_name));
}

// just the tokens occurring in the prefix
struct prefix_info
{
    cpp_attribute_list attributes;
    bool               is_constexpr = false;
    bool               is_virtual   = false;
    bool               is_explicit  = false;
    bool               is_friend    = false;
};

bool prefix_end(detail::cxtoken_stream& stream, const char* name, bool is_ctor_dtor)
{
    auto cur = stream.cur();
    // name can have multiple tokens if it is an operator
    if (!detail::skip_if(stream, name, true))
        return false;
    else if (stream.peek() == "," || stream.peek() == ">" || stream.peek() == ">>")
    {
        // argument to template parameters
        stream.set_cur(cur);
        return false;
    }
    else if (is_ctor_dtor)
    {
        // need to make sure it is not actually a class name
        if (stream.peek() == "::")
        {
            //  after name came "::", it is a class name
            stream.set_cur(cur);
            return false;
        }
        else if (stream.peek() == "<")
        {
            // after name came "<", it might be arguments for a class template,
            // or just a specialization
            // check if ( comes after the arguments
            detail::skip_brackets(stream);
            if (stream.peek() == "(")
            {
                // it was just a specialization, we're at the end
                stream.set_cur(cur);
                return true;
            }
            else
            {
                // class arguments
                stream.set_cur(cur);
                return false;
            }
        }
        else
            return true;
    }
    else if (std::strcmp(name, "operator") != 0 && stream.peek().kind() == CXToken_Identifier)
    {
        // can't be function name
        stream.set_cur(cur);
        return false;
    }
    else
        return true;
}

prefix_info parse_prefix_info(detail::cxtoken_stream& stream, const char* name, bool is_ctor_dtor)
{
    prefix_info result;

    while (!stream.done() && !prefix_end(stream, name, is_ctor_dtor))
    {
        if (detail::skip_if(stream, "constexpr"))
            result.is_constexpr = true;
        else if (detail::skip_if(stream, "virtual"))
            result.is_virtual = true;
        else if (detail::skip_if(stream, "explicit"))
            result.is_explicit = true;
        else
        {
            auto attributes = detail::parse_attributes(stream, true);
            result.attributes.insert(result.attributes.end(), attributes.begin(), attributes.end());
        }
    }
    DEBUG_ASSERT(!stream.done(), detail::parse_error_handler{}, stream.cursor(),
                 "unable to find end of function prefix");
    while (detail::skip_if(stream, ")"))
    { // function name can be enclosed in parentheses
    }

    auto attributes = detail::parse_attributes(stream);
    result.attributes.insert(result.attributes.end(), attributes.begin(), attributes.end());

    return result;
}

// just the tokens occurring in the suffix
struct suffix_info
{
    cpp_attribute_list              attributes;
    std::unique_ptr<cpp_expression> noexcept_condition;
    cpp_function_body_kind          body_kind;
    cpp_cv                          cv_qualifier  = cpp_cv_none;
    cpp_reference                   ref_qualifier = cpp_ref_none;
    cpp_virtual                     virtual_keywords;

    suffix_info(const CXCursor& cur)
    : body_kind(clang_isCursorDefinition(cur) ? cpp_function_definition : cpp_function_declaration)
    {}
};

cpp_cv parse_cv(detail::cxtoken_stream& stream)
{
    if (detail::skip_if(stream, "const"))
    {
        if (detail::skip_if(stream, "volatile"))
            return cpp_cv_const_volatile;
        else
            return cpp_cv_const;
    }
    else if (detail::skip_if(stream, "volatile"))
    {
        if (detail::skip_if(stream, "const"))
            return cpp_cv_const_volatile;
        else
            return cpp_cv_volatile;
    }
    else
        return cpp_cv_none;
}

cpp_reference parse_ref(detail::cxtoken_stream& stream)
{
    if (detail::skip_if(stream, "&"))
        return cpp_ref_lvalue;
    else if (detail::skip_if(stream, "&&"))
        return cpp_ref_rvalue;
    else
        return cpp_ref_none;
}

std::unique_ptr<cpp_expression> parse_noexcept(detail::cxtoken_stream&      stream,
                                               const detail::parse_context& context)
{
    if (!detail::skip_if(stream, "noexcept"))
        return nullptr;

    auto type = cpp_builtin_type::build(cpp_bool);
    if (stream.peek().value() != "(")
        return cpp_literal_expression::build(std::move(type), "true");

    auto closing = detail::find_closing_bracket(stream);

    detail::skip(stream, "(");
    auto expr = detail::parse_raw_expression(context, stream, closing, std::move(type));
    detail::skip(stream, ")");

    return expr;
}

cpp_function_body_kind parse_body_kind(detail::cxtoken_stream& stream, bool& pure_virtual)
{
    pure_virtual = false;
    if (detail::skip_if(stream, "default"))
        return cpp_function_defaulted;
    else if (detail::skip_if(stream, "delete"))
        return cpp_function_deleted;
    else if (detail::skip_if(stream, "0"))
    {
        pure_virtual = true;
        return cpp_function_declaration;
    }

    DEBUG_UNREACHABLE(detail::parse_error_handler{}, stream.cursor(),
                      "unexpected token for function body kind");
    return cpp_function_declaration;
}

void parse_body(detail::cxtoken_stream& stream, suffix_info& result, bool allow_virtual)
{
    auto pure_virtual = false;
    result.body_kind  = parse_body_kind(stream, pure_virtual);
    if (pure_virtual)
    {
        DEBUG_ASSERT(allow_virtual, detail::parse_error_handler{}, stream.cursor(),
                     "unexpected token");
        if (result.virtual_keywords)
            result.virtual_keywords.value() |= cpp_virtual_flags::pure;
        else
            result.virtual_keywords = cpp_virtual_flags::pure;
    }
}

// precondition: we've skipped the function parameters
suffix_info parse_suffix_info(detail::cxtoken_stream& stream, const detail::parse_context& context,
                              bool allow_qualifier, bool allow_virtual)
{
    suffix_info result(stream.cursor());

    // syntax: <attribute> <cv> <ref> <exception>
    result.attributes = detail::parse_attributes(stream);
    if (allow_qualifier)
    {
        result.cv_qualifier  = parse_cv(stream);
        result.ref_qualifier = parse_ref(stream);
    }

    if (detail::skip_if(stream, "throw"))
        // just because I can
        detail::skip_brackets(stream);
    result.noexcept_condition = parse_noexcept(stream, context);

    // check if we have leftovers of the return type
    // i.e.: `void (*foo(int a, int b) const)(int)`;
    //                                ^^^^^^- attributes
    //                                      ^^^^^^- leftovers
    // if we have a closing parenthesis, skip brackets
    if (detail::skip_if(stream, ")"))
        detail::skip_brackets(stream);

    // check for trailing return type
    if (detail::skip_if(stream, "->"))
    {
        // this is rather tricky to skip
        // so loop over all tokens and see if matching keytokens occur
        // note that this isn't quite correct
        // use a heuristic to skip brackets, which should be good enough
        while (!stream.done())
        {
            auto attributes = detail::parse_attributes(stream);
            if (!attributes.empty())
                result.attributes.insert(result.attributes.end(), attributes.begin(),
                                         attributes.end());
            else if (stream.peek() == "(" || stream.peek() == "[" || stream.peek() == "<")
                detail::skip_brackets(stream);
            else if (stream.peek() == "{")
                // begin of definition
                break;
            else if (detail::skip_if(stream, "override"))
            {
                DEBUG_ASSERT(allow_virtual, detail::parse_error_handler{}, stream.cursor(),
                             "unexpected token");
                if (result.virtual_keywords)
                    result.virtual_keywords.value() |= cpp_virtual_flags::override;
                else
                    result.virtual_keywords = cpp_virtual_flags::override;
            }
            else if (detail::skip_if(stream, "final"))
            {
                DEBUG_ASSERT(allow_virtual, detail::parse_error_handler{}, stream.cursor(),
                             "unexpected token");
                if (result.virtual_keywords)
                    result.virtual_keywords.value() |= cpp_virtual_flags::final;
                else
                    result.virtual_keywords = cpp_virtual_flags::final;
            }
            else if (detail::skip_if(stream, "="))
                parse_body(stream, result, allow_virtual);
            else
                stream.bump();
        }
        if (stream.peek() == "{" || stream.peek() == ":" || stream.peek() == "try")
            result.body_kind = cpp_function_definition;
    }
    else
    {
        // syntax: <virtuals> <body>
        if (detail::skip_if(stream, "override"))
        {
            DEBUG_ASSERT(allow_virtual, detail::parse_error_handler{}, stream.cursor(),
                         "unexpected token");
            result.virtual_keywords = cpp_virtual_flags::override;
            if (detail::skip_if(stream, "final"))
                result.virtual_keywords.value() |= cpp_virtual_flags::final;
        }
        else if (detail::skip_if(stream, "final"))
        {
            DEBUG_ASSERT(allow_virtual, detail::parse_error_handler{}, stream.cursor(),
                         "unexpected token");
            result.virtual_keywords = cpp_virtual_flags::final;
            if (detail::skip_if(stream, "override"))
                result.virtual_keywords.value() |= cpp_virtual_flags::override;
        }

        auto attributes = detail::parse_attributes(stream);
        if (!attributes.empty())
            result.attributes.insert(result.attributes.end(), attributes.begin(), attributes.end());

        if (detail::skip_if(stream, "="))
            parse_body(stream, result, allow_virtual);
        else if (detail::skip_if(stream, "{") || detail::skip_if(stream, ":")
                 || detail::skip_if(stream, "try"))
            result.body_kind = cpp_function_definition;
    }

    return result;
}

std::unique_ptr<cpp_entity> parse_cpp_function_impl(const detail::parse_context& context,
                                                    const CXCursor& cur, bool is_static,
                                                    bool is_friend)
{
    auto name = detail::get_cursor_name(cur);

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto prefix = parse_prefix_info(stream, name.c_str(), false);
    DEBUG_ASSERT(!prefix.is_virtual && !prefix.is_explicit, detail::parse_error_handler{}, cur,
                 "free function cannot be virtual or explicit");

    cpp_function::builder builder(name.c_str(),
                                  detail::parse_type(context, cur, clang_getCursorResultType(cur)));
    context.comments.match(builder.get(), cur);
    builder.get().add_attribute(prefix.attributes);

    add_parameters(context, builder, cur);
    if (clang_Cursor_isVariadic(cur))
        builder.is_variadic();
    builder.storage_class(cpp_storage_class_specifiers(
        detail::get_storage_class(cur)
        | (is_static ? cpp_storage_class_static : cpp_storage_class_none)));
    if (prefix.is_constexpr)
        builder.is_constexpr();

    skip_parameters(stream);

    auto suffix = parse_suffix_info(stream, context, false, false);
    builder.get().add_attribute(suffix.attributes);
    if (suffix.noexcept_condition)
        builder.noexcept_condition(std::move(suffix.noexcept_condition));

    if (is_templated_cursor(cur))
        return builder.finish(detail::get_entity_id(cur), suffix.body_kind,
                              parse_scope(cur, is_friend));
    else
        return builder.finish(*context.idx, detail::get_entity_id(cur), suffix.body_kind,
                              parse_scope(cur, is_friend));
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_function(const detail::parse_context& context,
                                                       const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_FunctionDecl
                     || clang_getTemplateCursorKind(cur) == CXCursor_FunctionDecl,
                 detail::assert_handler{});
    type_safe::optional<cpp_entity_ref> semantic_parent;
    return parse_cpp_function_impl(context, cur, false, is_friend);
}

std::unique_ptr<cpp_entity> detail::try_parse_static_cpp_function(
    const detail::parse_context& context, const CXCursor& cur)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_CXXMethod
                     || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod,
                 detail::assert_handler{});
    if (clang_CXXMethod_isStatic(cur))
        return parse_cpp_function_impl(context, cur, true, false);
    return nullptr;
}

namespace
{
bool overrides_function(const CXCursor& cur)
{
    CXCursor* overrides = nullptr;
    auto      num       = 0u;
    clang_getOverriddenCursors(cur, &overrides, &num);
    clang_disposeOverriddenCursors(overrides);
    return num != 0u;
}

cpp_virtual calculate_virtual(const CXCursor& cur, bool virtual_keyword,
                              const cpp_virtual& virtual_suffix)
{
    if (!clang_CXXMethod_isVirtual(cur) && !virtual_keyword && !virtual_suffix)
        return {};
    else if (clang_CXXMethod_isPureVirtual(cur))
    {
        // pure virtual function - all information in the suffix
        DEBUG_ASSERT(virtual_suffix.has_value() && virtual_suffix.value() & cpp_virtual_flags::pure,
                     detail::parse_error_handler{}, cur, "pure virtual not detected");
        return virtual_suffix;
    }
    else
    {
        // non-pure virtual function
        DEBUG_ASSERT(!virtual_suffix.has_value()
                         || !(virtual_suffix.value() & cpp_virtual_flags::pure),
                     detail::parse_error_handler{}, cur,
                     "pure virtual function detected, even though it isn't");
        // calculate whether it overrides
        auto overrides = !virtual_keyword
                         || (virtual_suffix.has_value()
                             && virtual_suffix.value() & cpp_virtual_flags::override)
                         || overrides_function(cur);

        // result are all the flags in the suffix
        auto result = virtual_suffix;
        if (!result)
            // make sure it isn't empty
            result.emplace();
        if (overrides)
            // make sure it contains the override flag
            result.value() |= cpp_virtual_flags::override;
        return result;
    }
}

template <class Builder>
auto set_qualifier(int, Builder& b, cpp_cv cv, cpp_reference ref)
    -> decltype(b.cv_ref_qualifier(cv, ref), true)
{
    b.cv_ref_qualifier(cv, ref);
    return true;
}

template <class Builder>
bool set_qualifier(short, Builder&, cpp_cv, cpp_reference)
{
    return false;
}

template <class Builder>
std::unique_ptr<cpp_entity> handle_suffix(const detail::parse_context& context, const CXCursor& cur,
                                          Builder& builder, detail::cxtoken_stream& stream,
                                          bool                                is_virtual,
                                          type_safe::optional<cpp_entity_ref> semantic_parent)
{
    auto allow_qualifiers = set_qualifier(0, builder, cpp_cv_none, cpp_ref_none);

    auto suffix = parse_suffix_info(stream, context, allow_qualifiers, true);
    builder.get().add_attribute(suffix.attributes);
    set_qualifier(0, builder, suffix.cv_qualifier, suffix.ref_qualifier);
    if (suffix.noexcept_condition)
        builder.noexcept_condition(move(suffix.noexcept_condition));
    if (auto virt = calculate_virtual(cur, is_virtual, suffix.virtual_keywords))
        builder.virtual_info(virt.value());

    if (is_templated_cursor(cur))
        return builder.finish(detail::get_entity_id(cur), suffix.body_kind,
                              std::move(semantic_parent));
    else
        return builder.finish(*context.idx, detail::get_entity_id(cur), suffix.body_kind,
                              std::move(semantic_parent));
}
} // namespace

std::unique_ptr<cpp_entity> detail::parse_cpp_member_function(const detail::parse_context& context,
                                                              const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_CXXMethod
                     || clang_getTemplateCursorKind(cur) == CXCursor_CXXMethod,
                 detail::assert_handler{});
    auto name = detail::get_cursor_name(cur);

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto prefix = parse_prefix_info(stream, name.c_str(), false);
    DEBUG_ASSERT(!prefix.is_explicit, detail::parse_error_handler{}, cur,
                 "member function cannot be explicit");

    cpp_member_function::builder builder(name.c_str(),
                                         detail::parse_type(context, cur,
                                                            clang_getCursorResultType(cur)));
    context.comments.match(builder.get(), cur);
    builder.get().add_attribute(prefix.attributes);
    add_parameters(context, builder, cur);
    if (clang_Cursor_isVariadic(cur))
        builder.is_variadic();

    if (prefix.is_constexpr)
        builder.is_constexpr();

    skip_parameters(stream);
    return handle_suffix(context, cur, builder, stream, prefix.is_virtual,
                         parse_scope(cur, is_friend));
}

std::unique_ptr<cpp_entity> detail::parse_cpp_conversion_op(const detail::parse_context& context,
                                                            const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_ConversionFunction
                     || clang_getTemplateCursorKind(cur) == CXCursor_ConversionFunction,
                 detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto prefix = parse_prefix_info(stream, "operator", false);

    // heuristic to find arguments tokens
    // skip forward, skipping inside brackets
    auto type_start = stream.cur();
    auto finished   = false;
    while (!stream.done() && !finished)
    {
        if (stream.peek() == "(")
        {
            if (detail::skip_if(stream, "(") && detail::skip_if(stream, ")"))
                finished = true;
            else
                detail::skip_brackets(stream);
        }
        else if (stream.peek() == "[")
            detail::skip_brackets(stream);
        else if (stream.peek() == "{")
            detail::skip_brackets(stream);
        else if (stream.peek() == "<")
            detail::skip_brackets(stream);
        else
            stream.bump();
    }
    DEBUG_ASSERT(finished, detail::parse_error_handler{}, cur,
                 "unable to find end of conversion op type");
    // bump arguments back
    stream.bump_back();
    stream.bump_back();
    auto type_end = stream.cur();

    // read the type
    stream.set_cur(type_start);
    auto type_spelling = detail::to_string(stream, type_end).as_string();

    // parse arguments again
    detail::skip(stream, "(");
    detail::skip(stream, ")");

    auto                       type = clang_getCursorResultType(cur);
    cpp_conversion_op::builder builder("operator " + type_spelling,
                                       detail::parse_type(context, cur, type));
    context.comments.match(builder.get(), cur);
    builder.get().add_attribute(prefix.attributes);
    if (prefix.is_explicit)
        builder.is_explicit();
    else if (prefix.is_constexpr)
        builder.is_constexpr();

    return handle_suffix(context, cur, builder, stream, prefix.is_virtual,
                         parse_scope(cur, is_friend));
}

std::unique_ptr<cpp_entity> detail::parse_cpp_constructor(const detail::parse_context& context,
                                                          const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_Constructor
                     || clang_getTemplateCursorKind(cur) == CXCursor_Constructor,
                 detail::assert_handler{});
    std::string name = detail::get_cursor_name(cur).c_str();
    auto        pos  = name.find('<');
    if (pos != std::string::npos)
        name.erase(pos);

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto prefix = parse_prefix_info(stream, name.c_str(), true);
    DEBUG_ASSERT(!prefix.is_virtual, detail::parse_error_handler{}, cur,
                 "constructor cannot be virtual");

    cpp_constructor::builder builder(name.c_str());
    context.comments.match(builder.get(), cur);
    add_parameters(context, builder, cur);
    builder.get().add_attribute(prefix.attributes);

    if (clang_Cursor_isVariadic(cur))
        builder.is_variadic();
    if (prefix.is_constexpr)
        builder.is_constexpr();
    else if (prefix.is_explicit)
        builder.is_explicit();

    skip_parameters(stream);

    auto suffix = parse_suffix_info(stream, context, false, false);
    builder.get().add_attribute(suffix.attributes);
    if (suffix.noexcept_condition)
        builder.noexcept_condition(std::move(suffix.noexcept_condition));

    if (is_templated_cursor(cur))
        return builder.finish(detail::get_entity_id(cur), suffix.body_kind,
                              parse_scope(cur, is_friend));
    else
        return builder.finish(*context.idx, detail::get_entity_id(cur), suffix.body_kind,
                              parse_scope(cur, is_friend));
}

std::unique_ptr<cpp_entity> detail::parse_cpp_destructor(const detail::parse_context& context,
                                                         const CXCursor& cur, bool is_friend)
{
    DEBUG_ASSERT(clang_getCursorKind(cur) == CXCursor_Destructor, detail::assert_handler{});

    detail::cxtokenizer    tokenizer(context.tu, context.file, cur);
    detail::cxtoken_stream stream(tokenizer, cur);

    auto prefix_info = parse_prefix_info(stream, "~", true);
    DEBUG_ASSERT(!prefix_info.is_constexpr && !prefix_info.is_explicit, detail::assert_handler{});

    auto                    name = std::string("~") + stream.get().c_str();
    cpp_destructor::builder builder(std::move(name));
    context.comments.match(builder.get(), cur);
    builder.get().add_attribute(prefix_info.attributes);

    detail::skip(stream, "(");
    detail::skip(stream, ")");
    return handle_suffix(context, cur, builder, stream, prefix_info.is_virtual,
                         parse_scope(cur, is_friend));
}
