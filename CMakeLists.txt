# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.3)
project(cppast VERSION 0.0)

# options
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(default_assertions ON)
else()
    set(default_assertions OFF)
endif()
option(CPPAST_ENABLE_ASSERTIONS "whether or not to enable internal assertions for the cppast library" ${default_assertions})
option(CPPAST_ENABLE_PRECONDITION_CHECKS "whether or not to enable precondition checks" ON)

option(CPPAST_BUILD_TEST "whether or not to build the tests" ON)

include(external/external.cmake)

add_subdirectory(src)
if(CPPAST_BUILD_TEST)
    add_subdirectory(test)
endif()

