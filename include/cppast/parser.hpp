// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSER_HPP_INCLUDED
#define CPPAST_PARSER_HPP_INCLUDED

#include <atomic>

#include <cppast/compile_config.hpp>
#include <cppast/cpp_file.hpp>
#include <cppast/cpp_preprocessor.hpp>
#include <cppast/diagnostic_logger.hpp>

namespace cppast
{
    class cpp_entity_index;

    /// Base class for a parser.
    ///
    /// It reads a C++ source file and creates the matching [cppast::cpp_file]().
    /// Derived classes can implement how the file is parsed.
    ///
    /// \requires A derived class must provide an alias `config` which is the corresponding derived class of the [cppast::compile_config]().
    class parser
    {
    public:
        parser(const parser&) = delete;
        parser& operator=(const parser&) = delete;

        virtual ~parser() noexcept = default;

        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        /// It can be `nullptr`, if there was an error or the specified file already registered in the index.
        /// \requires The dynamic type of `config` must match the required config type.
        /// \notes This function is thread safe.
        std::unique_ptr<cpp_file> parse(const cpp_entity_index& idx, std::string path,
                                        const compile_config& config) const
        {
            return do_parse(idx, std::move(path), config);
        }

        /// \returns Whether or not an error occurred during parsing.
        /// If that happens, the AST might be incomplete.
        bool error() const noexcept
        {
            return error_;
        }

        /// \effects Resets the error state.
        void reset_error() noexcept
        {
            error_ = false;
        }

    protected:
        /// \effects Creates it giving it a reference to the logger it uses.
        explicit parser(type_safe::object_ref<const diagnostic_logger> logger)
        : logger_(logger), error_(false)
        {
        }

        /// \returns A reference to the logger used.
        const diagnostic_logger& logger() const noexcept
        {
            return *logger_;
        }

        /// \effects Sets the error state.
        /// This must be called when an error or critical diagnostic is logged and the AST is incomplete.
        void set_error() const noexcept
        {
            error_ = true;
        }

    private:
        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        /// \requires The function must be thread safe.
        virtual std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                                   const compile_config& config) const = 0;

        type_safe::object_ref<const diagnostic_logger> logger_;
        mutable std::atomic<bool>                      error_;
    };

    /// A simple `FileParser` that parses all files synchronously.
    ///
    /// More advanced parsers could use a thread pool, for example.
    template <class Parser>
    class simple_file_parser
    {
        static_assert(std::is_base_of<cppast::parser, Parser>::value,
                      "Parser must be derived from cppast::parser");

    public:
        using parser = Parser;
        using config = typename Parser::config;

        /// \effects Creates a file parser populating the given index
        /// and using the parser created by forwarding the given arguments.
        template <typename... Args>
        explicit simple_file_parser(type_safe::object_ref<const cpp_entity_index> idx,
                                    Args&&... args)
        : parser_(std::forward<Args>(args)...), idx_(idx)
        {
        }

        /// \effects Parses the given file using the given configuration.
        /// \returns The parsed file or an empty optional, if a fatal error occurred.
        type_safe::optional_ref<const cpp_file> parse(std::string path, const config& c)
        {
            auto file = parser_.parse(*idx_, std::move(path), c);
            auto ptr  = file.get();
            if (file)
                files_.push_back(std::move(file));
            return type_safe::opt_ref(ptr);
        }

        /// \returns The result of [cppast::parser::error]().
        bool error() const noexcept
        {
            return parser_.error();
        }

        /// \effects Calls [cppast::parser::reset_error]().
        void reset_error() noexcept
        {
            parser_.reset_error();
        }

        /// \returns The index that is being populated.
        const cpp_entity_index& index() const noexcept
        {
            return *idx_;
        }

        /// \returns An iteratable object iterating over all the files that have been parsed so far.
        /// \exclude return
        detail::iteratable_intrusive_list<cpp_file> files() const noexcept
        {
            return type_safe::ref(files_);
        }

    private:
        Parser                                        parser_;
        detail::intrusive_list<cpp_file>              files_;
        type_safe::object_ref<const cpp_entity_index> idx_;
    };

    /// Parses multiple files using a given `FileParser`.
    ///
    /// \effects Will call the `parse()` function for each path specified in the `file_names`,
    /// using `get_confg` to determine the configuration.
    ///
    /// \requires `FileParser` must be a class with the following members:
    /// * `parser` - A typedef for the parser being used to do the parsing.
    /// * `config` - The same as `parser::config`.
    /// * `parse(path, config)` - Parses the given file with the configuration using its parser.
    /// The parsing can be executed in a separated thread, but then a copy of the configuration and path must be created.
    /// \requires `Range` must be some type that can be iterated in a range-based for loop.
    /// \requires `Configuration` must be a function that returns a configuration of type `FileParser::config` when given a path.
    /// \unique_name parse_files_basic
    /// \synopsis_return void
    template <class FileParser, class Range, class Configuration>
    auto parse_files(FileParser& parser, Range&& file_names, const Configuration& get_config) ->
        typename std::enable_if<std::is_same<typename std::decay<decltype(get_config(
                                                 *std::forward<Range>(file_names).begin()))>::type,
                                             typename FileParser::config>::value>::type
    {
        for (auto&& file : std::forward<Range>(file_names))
        {
            auto&& config = get_config(file);
            parser.parse(std::forward<decltype(file)>(file),
                         std::forward<decltype(config)>(config));
        }
    }

    /// Parses multiple files using a given `FileParser` and configuration.
    /// \effects Invokes [cppast::parse_files](standardese://parse_files_basic/) passing it the parser and file names,
    /// and a function that returns the same configuration for each file.
    template <class FileParser, class Range>
    void parse_files(FileParser& parser, Range&& file_names, typename FileParser::config config)
    {
        parse_files(parser, std::forward<Range>(file_names),
                    [&](const std::string&) { return config; });
    }

    /// Parses all files included by `file`.
    /// \effects For each [cppast::cpp_include_directive]() in file it will parse the included file.
    template <class FileParser>
    void resolve_includes(FileParser& parser, const cpp_file& file,
                          typename FileParser::config config)
    {
        for (auto& entity : file)
        {
            if (entity.kind() == cpp_include_directive::kind())
            {
                auto& include = static_cast<const cpp_include_directive&>(entity);
                parser.parse(include.full_path(), config);
            }
        }
    }
} // namespace cppast

#endif // CPPAST_PARSER_HPP_INCLUDED
