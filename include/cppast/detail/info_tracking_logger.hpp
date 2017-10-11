// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_INFO_TRACKING_LOGGER_HPP_INCLUDED
#define CPPAST_DETAIL_INFO_TRACKING_LOGGER_HPP_INCLUDED

#include <cppast/diagnostic_logger.hpp>
#include <type_safe/reference.hpp>

namespace cppast
{

namespace detail
{

/// Tracks statistics of usage of a given diagnostic logger
///
/// This class provides a proxy to collect statistics of a diagnostic
/// logger, such as the number of warnings logged, errors, etc
class info_tracking_logger : public cppast::diagnostic_logger
{
public:
    /// Creates the proxy with the given logger to track info from
    info_tracking_logger(const cppast::diagnostic_logger& logger);

    /// Returns whether an error has been logged or not
    bool error_logged() const;

    /// Returns the total number of error diagnostics logged
    std::size_t total_errors() const;

    /// Returns the total number of warning diagnostics logged
    std::size_t total_warnings() const;

    /// Returns the last error diagnostic logged, if any
    const type_safe::optional<const cppast::diagnostic>& last_error() const;

private:
    const cppast::diagnostic_logger& _logger;
    mutable std::size_t _total_errors, _total_warnings;
    mutable type_safe::optional<const cppast::diagnostic> _last_error;

    bool do_log(const char* source, const cppast::diagnostic& diagnostic) const override final;
};

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_ERROR_TRACKING_LOGGER_HPP_INCLUDED
