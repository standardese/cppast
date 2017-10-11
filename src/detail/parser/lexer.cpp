// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/detail/parser/lexer.hpp>
#include <cstdlib>

using namespace cppast;
using namespace cppast::detail::parser;

lexer::lexer(const diagnostic_logger& logger) :
    _logger(logger)
{}

const diagnostic_logger& lexer::logger() const
{
    return _logger;
}
