// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "tokenizer.hpp"

#include "libclang_visitor.hpp"
#include "parse_error.hpp"

using namespace cppast;

detail::token::token(const CXTranslationUnit& tu_unit, const CXToken& token)
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
        return clang_getLocationForOffset(tu, file, offset + inc);
    }

    class simple_tokenizer
    {
    public:
        explicit simple_tokenizer(const CXTranslationUnit& tu, const CXSourceRange& range,
                                  const CXCursor& cur)
        : tu_(tu)
        {
            clang_tokenize(tu, range, &tokens_, &no_);
            DEBUG_ASSERT(no_ >= 1u, detail::parse_error_handler{}, cur, "no tokens available");
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

    bool token_after_is(const CXTranslationUnit& tu, const CXFile& file, const CXCursor& cur,
                        const CXSourceLocation& loc, const char* token_str)
    {
        auto loc_after = get_next_location(tu, file, loc);

        simple_tokenizer tokenizer(tu, clang_getRange(loc, loc_after), cur);
        detail::cxstring spelling(clang_getTokenSpelling(tu, tokenizer[0u]));
        return spelling == token_str;
    }

    // clang_getCursorExtent() is somehow broken in various ways
    // this function returns the actual CXSourceRange that covers all parts required for parsing
    // might include more tokens
    // this function is the reason you shouldn't use libclang
    CXSourceRange get_extent(const CXTranslationUnit& tu, const CXFile& file, const CXCursor& cur)
    {
        auto extent = clang_getCursorExtent(cur);
        auto begin  = clang_getRangeStart(extent);
        auto end    = clang_getRangeEnd(extent);

        if (cursor_is_function(clang_getCursorKind(cur))
            || cursor_is_function(clang_getTemplateCursorKind(cur)))
        {
            auto range_shrunk = false;

            // if a function we need to remove the body
            // it does not need to be parsed
            detail::visit_children(cur, [&](const CXCursor& child) {
                if (clang_getCursorKind(child) == CXCursor_CompoundStmt
                    || clang_getCursorKind(child) == CXCursor_CXXTryStmt
                    || clang_getCursorKind(child) == CXCursor_InitListExpr)
                {
                    auto child_extent = clang_getCursorExtent(child);
                    end               = clang_getRangeStart(child_extent);
                    range_shrunk      = true;
                    return CXChildVisit_Break;
                }
                return CXChildVisit_Continue;
            });

            if (!range_shrunk && !token_after_is(tu, file, cur, end, ";"))
            {
                // we do not have a body, but it is not a declaration either
                do
                {
                    end = get_next_location(tu, file, end);
                } while (!token_after_is(tu, file, cur, end, ";"));
            }
            else if (clang_getCursorKind(cur) == CXCursor_CXXMethod)
                // necessary for some reason
                begin = get_next_location(tu, file, begin, -1);
        }
        else if (clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter
                 || clang_getCursorKind(cur) == CXCursor_NonTypeTemplateParameter
                 || clang_getCursorKind(cur) == CXCursor_TemplateTemplateParameter
                 || clang_getCursorKind(cur) == CXCursor_ParmDecl)
        {
            if (clang_getCursorKind(cur) == CXCursor_TemplateTypeParameter
                && token_after_is(tu, file, cur, end, "("))
            {
                // if you have decltype as default argument for a type template parameter
                // libclang doesn't include the parameters
                auto next = get_next_location(tu, file, end);
                auto prev = end;
                for (auto paren_count = 1; paren_count != 0;
                     next             = get_next_location(tu, file, next))
                {
                    if (token_after_is(tu, file, cur, next, "("))
                        ++paren_count;
                    else if (token_after_is(tu, file, cur, next, ")"))
                        --paren_count;
                    prev = next;
                }
                end = prev;
            }
        }
        else if (clang_getCursorKind(cur) == CXCursor_TypeAliasDecl
                 && !token_after_is(tu, file, cur, end, ";"))
        {
            // type alias tokens don't include everything
            do
            {
                end = get_next_location(tu, file, end);
            } while (!token_after_is(tu, file, cur, end, ";"));
            end = get_next_location(tu, file, end);
        }

        return clang_getRange(begin, end);
    }
}

detail::tokenizer::tokenizer(const CXTranslationUnit& tu, const CXFile& file, const CXCursor& cur)
{
    auto extent = get_extent(tu, file, cur);

    simple_tokenizer tokenizer(tu, extent, cur);
    tokens_.reserve(tokenizer.size());
    for (auto i = 0u; i != tokenizer.size(); ++i)
        tokens_.emplace_back(tu, tokenizer[i]);
}

void detail::skip(detail::token_stream& stream, const char* str)
{
    auto& token = stream.peek();
    DEBUG_ASSERT(token == str, parse_error_handler{}, stream.cursor(),
                 format("expected '", str, "', got '", token.c_str(), "'"));
    stream.bump();
}

bool detail::skip_if(detail::token_stream& stream, const char* str)
{
    auto& token = stream.peek();
    if (token != str)
        return false;
    stream.bump();
    return true;
}

detail::token_iterator detail::find_closing_bracket(detail::token_stream stream)
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
        close_bracket    = "<";
        template_bracket = true;
    }
    else
        DEBUG_UNREACHABLE(parse_error_handler{}, stream.cursor(),
                          format("expected a bracket, got '", stream.peek().c_str(), "'"));

    auto bracket_count = 1;
    auto paren_count   = 0; // internal nested parenthesis
    while (bracket_count != 0)
    {
        auto& cur = stream.get().value();
        if (paren_count == 0 && cur == open_bracket)
            ++bracket_count;
        else if (paren_count == 0 && cur == close_bracket)
            --bracket_count;
        else if (paren_count == 0 && template_bracket && cur == ">>")
            // maximal munch
            bracket_count -= 2u;
        else if (cur == "(")
            ++paren_count;
        else if (cur == ")")
            --paren_count;
    }
    stream.bump_back();
    DEBUG_ASSERT(paren_count == 0 && stream.peek().value() == close_bracket, parse_error_handler{},
                 stream.cursor(), "find_closing_bracket() internal parse error");
    return stream.cur();
}

void detail::skip_brackets(detail::token_stream& stream)
{
    auto closing = find_closing_bracket(stream);
    stream.set_cur(std::next(closing));
}

bool detail::skip_attribute(detail::token_stream& stream)
{
    if (skip_if(stream, "[") && stream.peek() == "[")
    {
        // C++11 attribute
        // [[<attribute>]]
        //  ^
        skip_brackets(stream);
        // [[<attribute>]]
        //               ^
        skip(stream, "]");
        return true;
    }
    else if (skip_if(stream, "__attribute__"))
    {
        // GCC/clang attributes
        // __attribute__(<attribute>)
        //              ^
        skip_brackets(stream);
        return true;
    }
    else if (skip_if(stream, "__declspec"))
    {
        // MSVC declspec
        // __declspec(<attribute>)
        //           ^
        skip_brackets(stream);
        return true;
    }

    return false;
}
