// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/info_tracking_logger.hpp>

using namespace cppast;
using namespace cppast::detail;

info_tracking_logger::info_tracking_logger(const diagnostic_logger& logger) :
    _logger(logger),
    _total_errors{0},
    _total_warnings{0},
    _last_error{type_safe::nullopt}
{}

bool info_tracking_logger::error_logged() const
{
    return _total_errors > 0;
}

std::size_t info_tracking_logger::total_errors() const
{
    return _total_errors;
}

std::size_t info_tracking_logger::total_warnings() const
{
    return _total_warnings;
}

const type_safe::optional<const diagnostic>& info_tracking_logger::last_error() const
{
    return _last_error;
}

bool info_tracking_logger::do_log(const char* source, const diagnostic& diagnostic) const
{
    if(_logger.log(source, diagnostic))
    {
        if(diagnostic.severity == severity::error)
        {
            _total_errors++;
            _last_error = diagnostic;
        }
        else if(diagnostic.severity == severity::warning)
        {
            _total_warnings++;
        }

        return true;
    }
    else
    {
        return false;
    }
}
