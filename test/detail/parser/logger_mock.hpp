// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_TEST_DETAIL_PARSER_LOGGER_MOCK_HPP_INCLUDED
#define CPPAST_TEST_DETAIL_PARSER_LOGGER_MOCK_HPP_INCLUDED

#include <cppast/diagnostic.hpp>
#include <trompeloeil.hpp>
#include <iostream>

namespace cppast
{

namespace test
{

class diagnostic_logger_mock_base : public cppast::diagnostic_logger
{
public:
    // Optionally log to stdout for debugging
    diagnostic_logger_mock_base(bool log_to_stdout = false) :
        _log_to_stout{log_to_stdout}
    {}

private:
    bool _log_to_stout;

    bool do_log(const char* source, const cppast::diagnostic& d) const override final
    {
        diagnostic_logged(
            source,
            d.severity,
            d.location.file.value_or(""),
            d.location.line.value_or(0),
            d.location.column.value_or(0),
            d.message
        );

        if(_log_to_stout)
        {
            std::cout << source << " " << d.location.to_string() << " " << d.message << "\n";
        }

        return true;
    }

    virtual void diagnostic_logged(
        const std::string& source,
        cppast::severity severity,
        const std::string& file,
        std::size_t line,
        std::size_t column,
        const std::string& message
    ) const = 0;
};

class diagnostic_logger_mock : public diagnostic_logger_mock_base
{
public:
    using diagnostic_logger_mock_base::diagnostic_logger_mock_base;

    MAKE_CONST_MOCK6(
        diagnostic_logged,
        void(const std::string&, cppast::severity, const std::string&, std::size_t, std::size_t, const std::string&),
        override final);
};

struct logger_context
{
    diagnostic_logger_mock logger;
    std::unique_ptr<trompeloeil::expectation> debug_logs_expectation;

    logger_context(bool log_to_stdout = false) :
        logger{log_to_stdout},
        debug_logs_expectation{
            // Allow debug logs by default
            NAMED_ALLOW_CALL(logger, diagnostic_logged(
                trompeloeil::_, // source
                cppast::severity::debug,
                trompeloeil::_, // file
                trompeloeil::_, // line
                trompeloeil::_, // column
                trompeloeil::_  // message
            ))
        }
    {
        logger.set_verbose(true);
    }
};

} // namespace cppast::test

} // namespace cppast

#endif // CPPAST_TEST_DETAIL_PARSER_LOGGER_MOCK_HPP_INCLUDED
