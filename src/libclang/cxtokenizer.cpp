// Copyright (C) 2017-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "cxtokenizer.hpp"

#include <cctype>

#include "libclang_visitor.hpp"
#include "parse_error.hpp"

using namespace cppast;

detail::cxtoken::cxtoken(const CXTranslationUnit& tu_unit, const CXToken& token)
: value_(clang_getTokenSpelling(tu_unit, token)), kind_(clang_getTokenKind(token))
{
}

namespace
{
    bool cursor_is_function(CXCursorKind kind)
    {
        return kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod
               || kind == CXCursor_Constructor || kind == CXCursor_Destructor
               || kind == CXCursor_ConversionFunction;
    }

    CXSourceLocation get_next_location(const CXTranslationUnit& tu, CXFile file,
                                       const CXSourceLocation& loc, int inc = 1)
    {
        unsigned offset;
        clang_getSpellingLocation(loc, nullptr, nullptr, nullptr, &offset);
        if (inc >= 0)
            offset += unsigned(inc);
        else
            offset -= unsigned(-inc);
        return clang_getLocationForOffset(tu, file, offset);
    }

    class simple_tokenizer
    {
    public:
        explicit simple_tokenizer(const CXTranslationUnit& tu, const CXSourceRange& range) : tu_(tu)
        {
            clang_tokenize(tu, range, &tokens_, &no_);
        }

        ~simple_tokenizer()
        {
            clang_disposeTokens(tu_, tokens_, no_);
        }

        simple_tokenizer(const simple_tokenizer&) = delete;
        simple_tokenizer& operator=(const simple_tokenizer&) = delete;

        unsigned size() const noexcept
        {
            return no_;
        }

        const CXToken& operator[](unsigned i) const noexcept
        {
            return tokens_[i];
        }

    private:
        CXTranslationUnit tu_;
        CXToken*          tokens_;
        unsigned          no_;
    };

    bool token_after_is(const CXTranslationUnit& tu, const CXFile& file,
                        const CXSourceLocation& loc, const char* token_str, int inc)
    {
        auto loc_after = get_next_location(tu, file, loc, inc);
        if (!clang_Location_isFromMainFile(loc_after))
            return false;

        simple_tokenizer tokenizer(tu, inc > 0 ? clang_getRange(loc, loc_after) :
                                                 clang_getRange(loc_after, loc));
        if (tokenizer.size() == 0u)
            return false;

        detail::cxstring spelling(clang_getTokenSpelling(tu, tokenizer[0u]));
        return spelling == token_str;
    }

    // clang_getCursorExtent() is somehow broken in various ways
    // this function returns the actual CXSourceRange that covers all parts required for parsing
    // might include more tokens
    // this function is the reason you shouldn't use libclang
    CXSourceRange get_extent(const CXTranslationUnit& tu, const CXFile& file, const CXCursor& cur,
                             bool& unmunch)
    {
        unmunch = false;

        auto extent = clang_getCursorExtent(cur);
        auto begin  = clang_getRangeStart(extent);
        auto end    = clang_getRangeEnd(extent);

        auto kind = clang_getCursorKind(cur);
        if (cursor_is_function(kind) || cursor_is_function(clang_getTemplateCursorKind(cur))
            || kind == CXCursor_VarDecl || kind == CXCursor_FieldDecl || kind == CXCursor_ParmDecl
            || kind == CXCursor_NonTypeTemplateParameter)
        {
            if (token_after_is(tu, file, begin, "]", -2)
                && token_after_is(tu, file, begin, "]", -3))
            {
                while (!token_after_is(tu, file, begin, "[", -1)
                       && !token_after_is(tu, file, begin, "[", -2))
                    begin = get_next_location(tu, file, begin, -1);

                begin = get_next_location(tu, file, begin, -3);
                DEBUG_ASSERT(token_after_is(tu, file, begin, "[", 0)
                                 && token_after_is(tu, file, get_next_location(tu, file, begin),
                                                   "[", 0),
                             detail::parse_error_handler{}, cur,
                             "error in pre-function attribute parsing");
            }
            else if (token_after_is(tu, file, begin, ")", -2))
            {
                // maybe alignas specifier
                auto save_begin = begin;

                auto paren_count = 1;
                begin            = get_next_location(tu, file, begin, -1);
                for (auto last_begin = begin; paren_count != 0; last_begin = begin)
                {
                    begin = get_next_location(tu, file, begin, -1);
                    if (token_after_is(tu, file, begin, "(", -1))
                        --paren_count;
                    else if (token_after_is(tu, file, begin, ")", -1))
                        ++paren_count;

                    DEBUG_ASSERT(!clang_equalLocations(last_begin, begin),
                                 detail::parse_error_handler{}, cur,
                                 "infinite loop in alignas parsing");
                }
                begin = get_next_location(tu, file, begin, -(int(std::strlen("alignas")) + 1));

                if (token_after_is(tu, file, begin, "alignas", 0))
                    begin = get_next_location(tu, file, begin, -1);
                else
                    begin = save_begin;
            }
        }

        if (cursor_is_function(kind) || cursor_is_function(clang_getTemplateCursorKind(cur)))
        {
            auto is_definition = false;
            // if a function we need to remove the body
            // it does not need to be parsed
            detail::visit_children(cur, [&](const CXCursor& child) {
                if (clang_getCursorKind(child) == CXCursor_CompoundStmt
                    || clang_getCursorKind(child) == CXCursor_CXXTryStmt
                    || clang_getCursorKind(child) == CXCursor_InitListExpr)
                {
                    auto child_extent = clang_getCursorExtent(child);
                    end               = clang_getRangeStart(child_extent);
                    is_definition     = true;
                }
            });

            if (!is_definition)
            {
                // i have no idea why this is necessary
                is_definition = token_after_is(tu, file, end, "{", 0)
                                || token_after_is(tu, file, end, "try", 0)
                                || token_after_is(tu, file, end, ":", 0);
                if (is_definition)
                    // need to extend range here to include the token
                    end = get_next_location(tu, file, end);
            }

            if (!is_definition && !token_after_is(tu, file, end, ";", 0))
            {
                // we do not have a body, but it is not a declaration either
                do
                {
                    end = get_next_location(tu, file, end);
                } while (!token_after_is(tu, file, end, ";", 0));
            }
            else if (kind == CXCursor_CXXMethod)
                // necessary for some reason
                begin = get_next_location(tu, file, begin, -1);
            else if (kind == CXCursor_Destructor && token_after_is(tu, file, end, ")", 0))
                // necessary for some other reason
                end = get_next_location(tu, file, end);
        }
        else if (kind == CXCursor_TemplateTypeParameter && token_after_is(tu, file, end, "(", 0))
        {
            // if you have decltype as default argument for a type template parameter
            // libclang doesn't include the parameters
            auto next = get_next_location(tu, file, end);
            auto prev = end;
            for (auto paren_count = 1; paren_count != 0; next = get_next_location(tu, file, next))
            {
                if (token_after_is(tu, file, next, "(", 0))
                    ++paren_count;
                else if (token_after_is(tu, file, next, ")", 0))
                    --paren_count;
                prev = next;
            }
#if CINDEX_VERSION_MINOR < 37
            end = prev;
#else
            end = next;
#endif
        }
        else if (kind == CXCursor_TemplateTemplateParameter
                 && token_after_is(tu, file, end, "<", 0))
        {
            // if you have a template template parameter in a template template parameter,
            // the tokens are all messed up, only contain the `template`

            // first: skip to closing angle bracket
            // luckily no need to handle expressions here
            auto next = get_next_location(tu, file, end, 2);
            for (auto angle_count = 1; angle_count != 0; next = get_next_location(tu, file, next))
            {
                if (token_after_is(tu, file, next, ">", 0))
                    --angle_count;
                else if (token_after_is(tu, file, next, ">>", 0))
                    angle_count -= 2;
                else if (token_after_is(tu, file, next, "<", 0))
                    ++angle_count;
            }

            // second: skip until end of parameter
            // no need to handle default, so look for '>' or ','
            while (!token_after_is(tu, file, next, ">", 0)
                   && !token_after_is(tu, file, next, ",", 0))
                next = get_next_location(tu, file, next);
            // now we found the proper end of the token
            end = get_next_location(tu, file, next, -1);
        }
        else if ((kind == CXCursor_TemplateTypeParameter
                  || kind == CXCursor_NonTypeTemplateParameter
                  || kind == CXCursor_TemplateTemplateParameter)
                 && token_after_is(tu, file, end, "...", 0))
        {
            // variadic tokens in unnamed parameter not included
            end = get_next_location(tu, file, end, 3);
            if (token_after_is(tu, file, end, ".", 0))
                // extra whitespace, so bump again
                // this should all go away once I redid the whole token thing...
                end = get_next_location(tu, file, end, 1);

            DEBUG_ASSERT(token_after_is(tu, file, end, ">", 0)
                             || token_after_is(tu, file, end, ",", 0),
                         detail::parse_error_handler{}, cur,
                         "unexpected token in variadic parameter workaround");
        }
        else if ((kind == CXCursor_TemplateTypeParameter
                  || kind == CXCursor_NonTypeTemplateParameter
                  || kind == CXCursor_TemplateTemplateParameter)
                 && !token_after_is(tu, file, end, ">", 0)
                 && !token_after_is(tu, file, end, ",", 0))
        {
            DEBUG_ASSERT(token_after_is(tu, file, get_next_location(tu, file, end, -2), ">>", 0),
                         detail::parse_error_handler{}, cur,
                         "unexpected token in maximal munch workaround");
            unmunch = true;
            // need to shrink range anyway
            end = get_next_location(tu, file, end, -1);
        }
        else if (kind == CXCursor_EnumDecl && !token_after_is(tu, file, end, ";", 0))
        {
            while (!token_after_is(tu, file, end, ";", 0))
                end = get_next_location(tu, file, end);
        }
        else if (kind == CXCursor_EnumConstantDecl && !token_after_is(tu, file, end, ",", 0))
        {
            // need to support attributes
            // just give up and extend the range to the range of the entire enum...
            auto parent = clang_getCursorLexicalParent(cur);
            end         = clang_getRangeEnd(clang_getCursorExtent(parent));
        }
        else if (kind == CXCursor_ParmDecl && !token_after_is(tu, file, end, "]", -1))
            // need to shrink range by one
            end = get_next_location(tu, file, end, -1);
        else if (kind == CXCursor_FieldDecl || kind == CXCursor_NonTypeTemplateParameter
                 || kind == CXCursor_TemplateTemplateParameter
#if CINDEX_VERSION_MINOR < 37
                 || clang_isExpression(kind) || kind == CXCursor_CXXBaseSpecifier
                 || kind == CXCursor_TemplateTypeParameter
#endif
        )
            // need to shrink range by one
            end = get_next_location(tu, file, end, -1);
        else if (kind == CXCursor_UnexposedDecl)
        {
            // include semicolon, if necessary
            if (token_after_is(tu, file, end, ";", 0))
                end = get_next_location(tu, file, end);
        }

        return clang_getRange(begin, end);
    }
}

detail::cxtokenizer::cxtokenizer(const CXTranslationUnit& tu, const CXFile& file,
                                 const CXCursor& cur)
{
    auto extent = get_extent(tu, file, cur, unmunch_);

    simple_tokenizer tokenizer(tu, extent);
    tokens_.reserve(tokenizer.size());
    for (auto i = 0u; i != tokenizer.size(); ++i)
        tokens_.emplace_back(tu, tokenizer[i]);
}

void detail::skip(detail::cxtoken_stream& stream, const char* str)
{
    if (*str)
    {
        // non-empty string
        DEBUG_ASSERT(!stream.done(), parse_error_handler{}, stream.cursor(),
                     format("expected '", str, "', got exhausted stream"));
        auto& token = stream.peek();
        DEBUG_ASSERT(token == str, parse_error_handler{}, stream.cursor(),
                     format("expected '", str, "', got '", token.c_str(), "'"));
        stream.bump();
    }
}

namespace
{
    bool starts_with(const char*& str, const detail::cxtoken& t)
    {
        if (std::strncmp(str, t.c_str(), t.value().length()) != 0)
            return false;
        str += t.value().length();
        while (*str == ' ' || *str == '\t')
            ++str;
        return true;
    }
}

bool detail::skip_if(detail::cxtoken_stream& stream, const char* str, bool multi_token)
{
    if (!*str)
        return true;
    else if (stream.done())
        return false;
    auto save = stream.cur();
    do
    {
        auto& token = stream.peek();
        if (!starts_with(str, token) || (!multi_token && *str != '\0'))
        {
            stream.set_cur(save);
            return false;
        }
        stream.bump();
    } while (multi_token && *str);
    return true;
}

namespace
{
    // whether or not the current angle bracket can be a comparison
    // note: this is a heuristic I hope works often enough
    bool is_comparison(CXTokenKind last_kind, const detail::cxtoken& cur, CXTokenKind next_kind)
    {
        if (cur == "<")
            return last_kind == CXToken_Literal;
        else if (cur == ">")
            return next_kind == CXToken_Literal;
        return false;
    }
}

detail::cxtoken_iterator detail::find_closing_bracket(detail::cxtoken_stream stream)
{
    auto        template_bracket = false;
    auto        open_bracket     = stream.peek().c_str();
    const char* close_bracket    = nullptr;
    if (skip_if(stream, "("))
        close_bracket = ")";
    else if (skip_if(stream, "{"))
        close_bracket = "}";
    else if (skip_if(stream, "["))
        close_bracket = "]";
    else if (skip_if(stream, "<"))
    {
        close_bracket    = ">";
        template_bracket = true;
    }
    else
        DEBUG_UNREACHABLE(parse_error_handler{}, stream.cursor(),
                          format("expected a bracket, got '", stream.peek().c_str(), "'"));

    auto bracket_count = 1;
    auto paren_count   = 0; // internal nested parenthesis
    auto last_token    = CXToken_Comment;
    while (!stream.done() && bracket_count != 0)
    {
        auto& cur = stream.get();
        if (paren_count == 0 && cur == open_bracket
            && !is_comparison(last_token, cur, stream.peek().kind()))
            ++bracket_count;
        else if (paren_count == 0 && cur == close_bracket
                 && !is_comparison(last_token, cur, stream.peek().kind()))
            --bracket_count;
        else if (paren_count == 0 && template_bracket && cur == ">>")
            // maximal munch
            bracket_count -= 2;
        else if (cur == "(" || cur == "{" || cur == "[")
            ++paren_count;
        else if (cur == ")" || cur == "}" || cur == "]")
            --paren_count;

        last_token = cur.kind();
    }
    stream.bump_back();
    // only check first parameter, token might be ">>"
    DEBUG_ASSERT(bracket_count == 0 && paren_count == 0
                     && stream.peek().value()[0] == close_bracket[0],
                 parse_error_handler{}, stream.cursor(),
                 "find_closing_bracket() internal parse error");
    return stream.cur();
}

void detail::skip_brackets(detail::cxtoken_stream& stream)
{
    auto closing = find_closing_bracket(stream);
    stream.set_cur(std::next(closing));
}

namespace
{
    type_safe::optional<std::string> parse_attribute_using(detail::cxtoken_stream& stream)
    {
        // using identifier :
        if (skip_if(stream, "using"))
        {
            DEBUG_ASSERT(stream.peek().kind() == CXToken_Identifier, detail::parse_error_handler{},
                         stream.cursor(), "expected identifier");
            auto scope = stream.get().value().std_str();
            skip(stream, ":");

            return scope;
        }
        else
            return type_safe::nullopt;
    }

    cpp_attribute_kind get_attribute_kind(const std::string& name)
    {
        if (name == "carries_dependency")
            return cpp_attribute_kind::carries_dependency;
        else if (name == "deprecated")
            return cpp_attribute_kind::deprecated;
        else if (name == "fallthrough")
            return cpp_attribute_kind::fallthrough;
        else if (name == "maybe_unused")
            return cpp_attribute_kind::maybe_unused;
        else if (name == "nodiscard")
            return cpp_attribute_kind::nodiscard;
        else if (name == "noreturn")
            return cpp_attribute_kind::noreturn;
        else
            return cpp_attribute_kind::unknown;
    }

    cpp_token_string parse_attribute_arguments(detail::cxtoken_stream& stream)
    {
        auto end = find_closing_bracket(stream);
        skip(stream, "(");

        auto arguments = detail::to_string(stream, end);

        stream.set_cur(end);
        skip(stream, ")");

        return arguments;
    }

    cpp_attribute parse_attribute_token(detail::cxtoken_stream&          stream,
                                        type_safe::optional<std::string> scope)
    {
        // (identifier ::)_opt identifier ( '(' some tokens ')' )_opt ..._opt

        // parse name
        DEBUG_ASSERT(stream.peek().kind() == CXToken_Identifier, detail::parse_error_handler{},
                     stream.cursor(), "expected identifier");
        auto name = stream.get().value().std_str();
        if (skip_if(stream, "::"))
        {
            // name was actually a scope, so parse name again
            DEBUG_ASSERT(!scope, detail::parse_error_handler{}, stream.cursor(),
                         "attribute using + scope not allowed");
            scope = std::move(name);

            DEBUG_ASSERT(stream.peek().kind() == CXToken_Identifier, detail::parse_error_handler{},
                         stream.cursor(), "expected identifier");
            name = stream.get().value().std_str();
        }

        // parse arguments
        type_safe::optional<cpp_token_string> arguments;
        if (stream.peek() == "(")
            arguments = parse_attribute_arguments(stream);

        // parse variadic token
        auto is_variadic = skip_if(stream, "...");

        // get kind
        auto kind = get_attribute_kind(name);
        if (!scope && kind != cpp_attribute_kind::unknown)
            return cpp_attribute(kind, std::move(arguments));
        else
            return cpp_attribute(std::move(scope), std::move(name), std::move(arguments),
                                 is_variadic);
    }

    bool parse_attribute_impl(cpp_attribute_list& result, detail::cxtoken_stream& stream)
    {
        if (skip_if(stream, "[") && stream.peek() == "[")
        {
            // C++11 attribute
            // [[<attribute>]]
            //  ^
            skip(stream, "[");

            auto scope = parse_attribute_using(stream);
            while (!skip_if(stream, "]"))
            {
                auto attribute = parse_attribute_token(stream, scope);
                result.push_back(std::move(attribute));
                detail::skip_if(stream, ",");
            }

            // [[<attribute>]]
            //               ^
            skip(stream, "]");
            return true;
        }
        else if (skip_if(stream, "alignas"))
        {
            // alignas specifier
            // alignas(<some arguments>)
            //        ^
            auto arguments = parse_attribute_arguments(stream);
            result.push_back(cpp_attribute(cpp_attribute_kind::alignas_, std::move(arguments)));
        }
        else if (skip_if(stream, "__attribute__") && stream.peek() == "(")
        {
            // GCC/clang attributes
            // __attribute__((<attribute>))
            //              ^^
            skip(stream, "(");
            skip(stream, "(");

            auto scope = parse_attribute_using(stream);
            while (!skip_if(stream, ")"))
            {
                auto attribute = parse_attribute_token(stream, scope);
                result.push_back(std::move(attribute));
                detail::skip_if(stream, ",");
            }

            skip(stream, ")");
            return true;
        }
        else if (skip_if(stream, "__declspec"))
        {
            // MSVC declspec
            // __declspec(<attribute>)
            //           ^
            skip(stream, "(");
            auto scope = parse_attribute_using(stream);
            while (!skip_if(stream, ")"))
            {
                auto attribute = parse_attribute_token(stream, scope);
                result.push_back(std::move(attribute));
                detail::skip_if(stream, ",");
            }

            return true;
        }

        return false;
    }
}

cpp_attribute_list detail::parse_attributes(detail::cxtoken_stream& stream, bool skip_anway)
{
    cpp_attribute_list result;

    while (parse_attribute_impl(result, stream))
        skip_anway = false;

    if (skip_anway)
        stream.bump();

    return result;
}

namespace
{
    cpp_token_kind get_kind(const detail::cxtoken& token)
    {
        switch (token.kind())
        {
        case CXToken_Punctuation:
            return cpp_token_kind::punctuation;
        case CXToken_Keyword:
            return cpp_token_kind::keyword;
        case CXToken_Identifier:
            return cpp_token_kind::identifier;

        case CXToken_Literal:
        {
            auto spelling = token.value().std_str();
            if (spelling.find('.') != std::string::npos)
                return cpp_token_kind::float_literal;
            else if (std::isdigit(spelling.front()))
                return cpp_token_kind::int_literal;
            else if (spelling.back() == '\'')
                return cpp_token_kind::char_literal;
            else
                return cpp_token_kind::string_literal;
        }

        case CXToken_Comment:
            break;
        }

        DEBUG_UNREACHABLE(detail::assert_handler{});
        return cpp_token_kind::punctuation;
    }
}

cpp_token_string detail::to_string(cxtoken_stream& stream, cxtoken_iterator end)
{
    cpp_token_string::builder builder;

    while (stream.cur() != end)
    {
        auto& token = stream.get();
        builder.add_token(cpp_token(get_kind(token), token.c_str()));
    }

    if (stream.unmunch())
        builder.unmunch();

    return builder.finish();
}

bool detail::append_scope(detail::cxtoken_stream& stream, std::string& scope)
{
    // add identifiers and "::" to current scope name,
    // clear if there is any other token in between, or mismatched combination
    if (stream.peek().kind() == CXToken_Identifier)
    {
        if (!scope.empty() && scope.back() != ':')
            scope.clear();
        scope += stream.get().c_str();
    }
    else if (stream.peek() == "::")
    {
        if (!scope.empty() && scope.back() == ':')
            scope.clear();
        scope += stream.get().c_str();
    }
    else if (stream.peek() == "<")
    {
        auto iter = detail::find_closing_bracket(stream);
        scope += detail::to_string(stream, iter).as_string();
        if (!detail::skip_if(stream, ">>"))
            detail::skip(stream, ">");
        scope += ">";
    }
    else
    {
        scope.clear();
        return false;
    }
    return true;
}
