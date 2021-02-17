# Copyright (C) 2017-2018 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# Fetch catch.
message(STATUS "Fetching catch")
include(FetchContent)
FetchContent_Declare(catch URL https://github.com/catchorg/Catch2/archive/v2.13.4.zip)
FetchContent_MakeAvailable(catch)

set(tests
        code_generator.cpp
        cpp_alias_template.cpp
        cpp_attribute.cpp
        cpp_class.cpp
        cpp_class_template.cpp
        cpp_enum.cpp
        cpp_friend.cpp
        cpp_function.cpp
        cpp_function_template.cpp
        cpp_language_linkage.cpp
        cpp_member_function.cpp
        cpp_member_variable.cpp
        cpp_namespace.cpp
        cpp_preprocessor.cpp
        cpp_static_assert.cpp
        cpp_template_parameter.cpp
        cpp_token.cpp
        cpp_type_alias.cpp
        cpp_variable.cpp
        integration.cpp
        libclang_parser.cpp
        parser.cpp
        preprocessor.cpp
        visitor.cpp)

# generate list of source files for the self parsing test
get_target_property(files cppast SOURCES)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cppast_files.hpp "// list of cppast source file includes\n")
foreach(file ${files})
    file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/cppast_files.hpp "\"${CMAKE_CURRENT_SOURCE_DIR}/../src/${file}\",\n")
endforeach()

add_executable(cppast_test test.cpp test_parser.hpp ${tests})
target_include_directories(cppast_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(cppast_test PUBLIC cppast Catch2)
target_compile_definitions(cppast_test PUBLIC CPPAST_INTEGRATION_FILE="${CMAKE_CURRENT_SOURCE_DIR}/integration.cpp"
                                              CPPAST_COMPILE_COMMANDS="${CMAKE_BINARY_DIR}")

add_test(NAME unit_test COMMAND cppast_test "~[integration]")
add_test(NAME integration_test COMMAND cppast_test "[integration]")

if(CPPAST_TEST_GCOV AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    setup_target_for_coverage(
        DIRECTORY ${CMAKE_SOURCE_DIR}
        NAME cppast_coverage
        SOURCES src/* include/*
        COLLECT_EXISTING
    )
    target_link_libraries(cppast_test PRIVATE gcov)
endif()
