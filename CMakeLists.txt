# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.3)
project(cppast VERSION 0.0)

# options
option(CPPAST_BUILD_TEST "whether or not to build the tests" ON)

include(external/external.cmake)

add_subdirectory(src)
if(CPPAST_BUILD_TEST)
    add_subdirectory(test)
endif()

