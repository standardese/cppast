// Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_PARSER_HPP_INCLUDED
#define CPPAST_PARSER_HPP_INCLUDED

#include <cppast/compile_config.hpp>
#include <cppast/cpp_file.hpp>

namespace cppast
{
    class cpp_entity_index;

    /// Base class for a parser.
    ///
    /// It reads a C++ source file and creates the matching [cppast::cpp_file]().
    class parser
    {
    public:
        parser(const parser&) = delete;
        parser& operator=(const parser&) = delete;

        virtual ~parser() noexcept = default;

        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        /// \requires The dynamic type of `config` must match the required config type.
        std::unique_ptr<cpp_file> parse(const cpp_entity_index& idx, std::string path,
                                        const compile_config& config) const
        {
            return do_parse(idx, std::move(path), config);
        }

    protected:
        parser() = default;

    private:
        /// \effects Parses the given file.
        /// \returns The [cppast::cpp_file]() object describing it.
        virtual std::unique_ptr<cpp_file> do_parse(const cpp_entity_index& idx, std::string path,
                                                   const compile_config& config) const = 0;
    };
} // namespace cppast

#endif // CPPAST_PARSER_HPP_INCLUDED
