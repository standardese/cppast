# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# download catch
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
    file(DOWNLOAD https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
endif()

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

add_executable(cppast_test test.cpp test_parser.hpp ${tests})
target_include_directories(cppast_test PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(cppast_test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../src)
target_link_libraries(cppast_test PUBLIC cppast)
set_target_properties(cppast_test PROPERTIES CXX_STANDARD 11)

enable_testing()
add_test(NAME test COMMAND cppast_test)

if(CPPAST_TEST_GCOV AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    setup_target_for_coverage(
        DIRECTORY ${CMAKE_SOURCE_DIR}
        NAME cppast_coverage
        SOURCES src/* include/*
        COLLECT_EXISTING
    )
    target_link_libraries(cppast_test PRIVATE gcov)
endif()
