// Copyright (C) 2017-2022 Jonathan Müller and cppast contributors
// SPDX-License-Identifier: MIT

#ifndef CPPAST_CODE_GENERATOR_HPP_INCLUDED
#define CPPAST_CODE_GENERATOR_HPP_INCLUDED

#include <cstring>

#include <type_safe/flag_set.hpp>
#include <type_safe/index.hpp>

#include <cppast/cpp_entity.hpp>
#include <cppast/cpp_entity_ref.hpp>

namespace cppast
{
/// A simple string view implementation, like [std::string_view]().
///
/// It "views" - stores a pointer to - some kind of string.
class string_view
{
public:
    /// \effects Creates it viewing the [std::string]().
    string_view(const std::string& str) noexcept : str_(str.c_str()), length_(str.length()) {}

    /// \effects Creates it viewing the C string `str`.
    string_view(const char* str) noexcept : str_(str), length_(std::strlen(str)) {}

    /// \effects Creates it viewing the C string literal.
    template <std::size_t Size>
    constexpr string_view(const char (&str)[Size]) noexcept : str_(str), length_(Size - 1u)
    {}

    /// \returns The number of characters.
    constexpr std::size_t length() const noexcept
    {
        return length_;
    }

    /// \returns The C string.
    constexpr const char* c_str() const noexcept
    {
        return str_;
    }

    /// \returns The character at the given index.
    /// \requires `i < length()`.
    char operator[](type_safe::index_t i) const noexcept
    {
        return type_safe::at(str_, i);
    }

private:
    const char* str_;
    std::size_t length_;
};

/// \exclude
namespace detail
{
    template <typename Tag>
    class semantic_string_view
    {
    public:
        template <typename T>
        explicit semantic_string_view(T&& obj, decltype(string_view(std::declval<T>()), 0) = 0)
        : str_(std::forward<T>(obj))
        {}

        const string_view& str() const noexcept
        {
            return str_;
        }

    private:
        string_view str_;
    };
} // namespace detail

/// A special [cppast::string_view]() representing a C++ keyword token.
using keyword = detail::semantic_string_view<struct keyword_tag>;

/// A special [cppast::string_view]() representing a C++ identifier token.
using identifier = detail::semantic_string_view<struct identifier_tag>;

/// A special [cppast::string_view]() representing a C++ string or character literal token.
using string_literal = detail::semantic_string_view<struct str_literal_tag>;

/// A special [cppast::string_view]() representing a C++ integer literal token.
using int_literal = detail::semantic_string_view<struct int_literal_tag>;

/// A special [cppast::string_view]() representing a C++ floating point literal token.
using float_literal = detail::semantic_string_view<struct float_literal_tag>;

/// A special [cppast::string_view]() representing a C++ punctuation token like `.` or `(`.
using punctuation = detail::semantic_string_view<struct punctuation_tag>;

/// A special [cppast::string_view]() representing a C++ preprocessor token.
using preprocessor_token = detail::semantic_string_view<struct preprocessor_tag>;

/// A special [cppast::string_view]() representing a comment.
using comment = detail::semantic_string_view<struct comment_tag>;

/// A special [cppast::string_view]() representing a sequence of unknown C++ tokens.
using token_seq = detail::semantic_string_view<struct token_seq_tag>;

/// Tag type to represent an end-of-line character.
const struct newl_t
{
} newl{};

/// Tag type to represent a single space character.
const struct whitespace_t
{
} whitespace{};

/// Flags that control the code formatting.
///
/// If a flag is set, it adds additional whitespace.
/// If no flags are set, it will only add the whitespace necessary to separate tokens.
enum class formatting_flags
{
    brace_nl, //< Set to put the opening braces on a new line.
    brace_ws, //< Set to put the opening brace at the end of the line after whitespace.

    ptr_ref_var, //< Set to put pointers and references at the variable name (default is type).

    comma_ws,    //< Set to put whitespace after a comma.
    bracket_ws,  //< Set to put whitespace inside brackets.
    operator_ws, //< Set to put whitespace around operators.

    _flag_set_size, //< \exclude
};

/// A set of formatting flags.
using formatting = type_safe::flag_set<formatting_flags>;

/// Base class to control the code generation.
///
/// Inherit from it to customize how a [cppast::cpp_entity]() is printed
/// by [cppast::generate_code]().
class code_generator
{
public:
    code_generator(const code_generator&) = delete;

    code_generator& operator=(const code_generator&) = delete;

    virtual ~code_generator() noexcept = default;

    /// Flags that control the generation.
    enum generation_flags
    {
        custom,                     //< A custom output is written.
        exclude,                    //< Exclude the entire entity.
        exclude_return,             //< Exclude the return type of a function entity.
        exclude_target,             //< Exclude the underlying entity of an alias (e.g. typedef).
        exclude_noexcept_condition, //< Exclude the condition of a noexcept.`
        declaration,                //< Only write declaration.
        /// For a macro, it won't show the replacement if this flag is set
        _flag_set_size, //< \exclude
    };

    /// Options that control the generation.
    using generation_options = type_safe::flag_set<generation_flags>;

    /// Sentinel type used to output a given entity.
    class output
    {
    public:
        /// \effects Creates it giving the generator and the entity.
        /// \requires No other output must be active for the same code generator.
        explicit output(type_safe::object_ref<code_generator>   gen,
                        type_safe::object_ref<const cpp_entity> e,
                        cppast::cpp_access_specifier_kind       access)
        : gen_(gen), e_(e), options_(gen->do_get_options(*e, access))
        {
            gen_->on_begin(*this, *e_);
        }

        ~output() noexcept
        {
            if (*this)
                gen_->on_end(*this, *e_);
        }

        output(const output&) = delete;

        output& operator=(const output&) = delete;

        /// \returns Whether or not the `on_XXX` function returned something other than `exclude` or
        /// `custom`.
        explicit operator bool() const noexcept
        {
            return !options_.is_set(exclude) && !options_.is_set(custom);
        }

        /// \returns The generation options.
        generation_options options() const
        {
            return options_;
        }

        /// \returns The generation options for the given entity.
        generation_options options(const cpp_entity& e, cpp_access_specifier_kind access) const
        {
            return gen_->do_get_options(e, access);
        }

        /// \returns The formatting.
        cppast::formatting formatting() const
        {
            return gen_->do_get_formatting();
        }

        /// \returns Whether or not the definition should be generated as well.
        bool generate_definition() const noexcept
        {
            return !(options_ & declaration);
        }

        /// \returns A reference to the generator.
        type_safe::object_ref<code_generator> generator() const noexcept
        {
            return gen_;
        }

        /// \effects Calls `on_container_end()`.
        void container_end() const noexcept
        {
            gen_->on_container_end(*this, *e_);
        }

        /// \effects Call `do_indent()` followed by `do_write_newline()` (if `print_newline` is
        /// `true`).
        void indent(bool print_newline = true) const noexcept
        {
            gen_->do_indent();
            if (print_newline)
                gen_->do_write_newline();
        }

        /// \effects Calls `do_unindent()`.
        void unindent() const noexcept
        {
            gen_->do_unindent();
        }

        /// \effects Calls `func(*this)`.
        const output& operator<<(void (*func)(const output&)) const
        {
            func(*this);
            return *this;
        }

        /// \effects Calls `do_write_keyword()`.
        const output& operator<<(const keyword& k) const
        {
            gen_->do_write_keyword(k.str());
            return *this;
        }

        /// \effects Calls `do_write_identifier()`.
        const output& operator<<(const identifier& ident) const
        {
            gen_->do_write_identifier(ident.str());
            return *this;
        }

        /// \effects Calls `do_write_reference()`.
        template <typename T, class Predicate>
        const output& operator<<(const basic_cpp_entity_ref<T, Predicate>& ref) const
        {
            was_excluded_ = !gen_->do_write_reference(ref.id(), ref.name());
            return *this;
        }

        /// \returns Whether or not the last reference was excluded.
        bool was_reference_excluded() const
        {
            return was_excluded_;
        }

        /// \effects Calls `do_write_punctuation()`.
        const output& operator<<(const punctuation& punct) const
        {
            gen_->do_write_punctuation(punct.str());
            return *this;
        }

        /// \effects Calls `do_write_str_literal`.
        const output& operator<<(const string_literal& lit) const
        {
            gen_->do_write_str_literal(lit.str());
            return *this;
        }

        /// \effects Calls `do_write_int_literal()`.
        const output& operator<<(const int_literal& lit) const
        {
            gen_->do_write_int_literal(lit.str());
            return *this;
        }

        /// \effects Calls `do_write_float_literal()`.
        const output& operator<<(const float_literal& lit) const
        {
            gen_->do_write_float_literal(lit.str());
            return *this;
        }

        /// \effects Calls `do_write_preprocessor()`.
        const output& operator<<(const preprocessor_token& tok) const
        {
            gen_->do_write_preprocessor(tok.str());
            return *this;
        }

        /// \effects Calls `do_write_comment()`.
        const output& operator<<(const comment& c) const
        {
            gen_->do_write_comment(c.str());
            return *this;
        }

        /// \effects Calls `do_write_token_seq()`.
        const output& operator<<(const token_seq& seq) const
        {
            gen_->do_write_token_seq(seq.str());
            return *this;
        }

        /// \effects Calls `do_write_excluded()`.
        const output& excluded(const cpp_entity& e) const
        {
            gen_->do_write_excluded(e);
            return *this;
        }

        /// \effects Calls `do_write_newline()`.
        const output& operator<<(newl_t) const
        {
            gen_->do_write_newline();
            return *this;
        }

        /// \effects Calls `do_write_whitespace()`.
        const output& operator<<(whitespace_t) const
        {
            gen_->do_write_whitespace();
            return *this;
        }

    private:
        type_safe::object_ref<code_generator>   gen_;
        type_safe::object_ref<const cpp_entity> e_;
        generation_options                      options_;
        mutable bool                            was_excluded_ = false;
    };

protected:
    code_generator() noexcept = default;

    /// \returns The entity whose code is being generated with [cppast::generate_code()]().
    const cpp_entity& main_entity() const noexcept
    {
        return main_entity_.value();
    }

    /// \effects Generates the code for the specified entity.
    /// It can be used to generate additional entities while generating another one.
    /// \returns Whether or not any code was generated.
    /// \notes This does not affect the main entity, but otherwise behaves just like
    /// [cppast::generate_code()]().
    bool generate_code(const cpp_entity& entity);

private:
    /// \returns The formatting options that should be used.
    /// The base class version has no flags set.
    virtual formatting do_get_formatting() const
    {
        return {};
    }

    /// \returns The generation options for that entity with the given access specifier.
    /// If an entity is not part of a class the access specifier is alwasy [cppast::cpp_public]().
    /// The base class version always returns no special options.
    virtual generation_options do_get_options(const cpp_entity&                 e,
                                              cppast::cpp_access_specifier_kind access)
    {
        (void)e;
        (void)access;
        return {};
    }

    /// \effects Will be invoked before code of an entity is generated.
    /// The base class version has no effect.
    virtual void on_begin(const output& out, const cpp_entity& e)
    {
        (void)out;
        (void)e;
    }

    /// \effects Will be invoked after all code of an entity has been generated.
    /// The base class version has no effect.
    virtual void on_end(const output& out, const cpp_entity& e)
    {
        (void)out;
        (void)e;
    }

    /// \effects Will be invoked after all children of a container have been generated.
    /// It can be used to inject additional children.
    /// The base class version has no effect.
    virtual void on_container_end(const output& out, const cpp_entity& e)
    {
        (void)out;
        (void)e;
    }

    /// \effects Will be invoked when the indentation level should be increased by one.
    /// The level change must be applied on the next call to `do_write_newline()`.
    virtual void do_indent() = 0;

    /// \effects Will be invoked when the indentation level should be decreased by one.
    /// The level change must be applied immediately if nothing else has been written on the current
    /// line.
    virtual void do_unindent() = 0;

    /// \effects Writes the given token sequence.
    virtual void do_write_token_seq(string_view tokens) = 0;

    /// \effects Writes the specified special token.
    /// The base class version simply forwards to `do_write_token_seq()`.
    /// \returns `do_write_reference()` returns `false`, if the reference was excluded.
    /// \notes This is useful for syntax highlighting, for example.
    /// \group write
    virtual void do_write_keyword(string_view keyword)
    {
        do_write_token_seq(keyword);
    }
    /// \group write
    virtual void do_write_identifier(string_view identifier)
    {
        do_write_token_seq(identifier);
    }
    /// \group write
    virtual bool do_write_reference(type_safe::array_ref<const cpp_entity_id> id, string_view name)
    {
        (void)id;
        do_write_token_seq(name);
        return true;
    }
    /// \group write
    virtual void do_write_punctuation(string_view punct)
    {
        do_write_token_seq(punct);
    }
    /// \group write
    virtual void do_write_str_literal(string_view str)
    {
        do_write_token_seq(str);
    }
    /// \group write
    virtual void do_write_int_literal(string_view str)
    {
        do_write_token_seq(str);
    }
    /// \group write
    virtual void do_write_float_literal(string_view str)
    {
        do_write_token_seq(str);
    }
    /// \group write
    virtual void do_write_preprocessor(string_view punct)
    {
        do_write_token_seq(punct);
    }
    /// \group write
    virtual void do_write_comment(string_view c)
    {
        do_write_token_seq(c);
    }

    /// \effects Writes a string for an excluded target or return type for the given entity.
    /// The base class version writes the identifier `excluded`.
    virtual void do_write_excluded(const cpp_entity& e)
    {
        (void)e;
        do_write_identifier("excluded");
    }

    /// \effects Writes a newline.
    /// It is guaranteed that this is the only way a newline will be printed.
    /// The base class forwards to `do_write_token_seq()`.
    virtual void do_write_newline()
    {
        do_write_token_seq("\n");
    }
    /// \effects Writes a whitespace character.
    /// It will be invoked only where a whitespace is truly needed,
    /// like between two keywords.
    /// The base class forwards to `do_write_token_seq()`.
    virtual void do_write_whitespace()
    {
        do_write_token_seq(" ");
    }

    type_safe::optional_ref<const cpp_entity> main_entity_;

    friend bool generate_code(code_generator& generator, const cpp_entity& e);
};

/// Generates code for the given entity.
///
/// How the code is generated is customized by the generator.
///
/// \returns Whether or not any code was actually written.
bool generate_code(code_generator& generator, const cpp_entity& e);

/// \exclude
namespace detail
{
    void write_template_arguments(
        code_generator::output&                                                output,
        type_safe::optional<type_safe::array_ref<const cpp_template_argument>> arguments);

    void write_token_string(code_generator::output& output, const cpp_token_string& tokens);
} // namespace detail
} // namespace cppast

#endif // CPPAST_CODE_GENERATOR_HPP_INCLUDED
