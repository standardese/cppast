# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

#
# install type safe
#
find_package(type_safe QUIET)
if(NOT type_safe_FOUND)
    message(STATUS "Installing type_safe via submodule")
    execute_process(COMMAND git submodule update --init -- external/type_safe
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/type_safe EXCLUDE_FROM_ALL)
endif()

#
# install the tiny-process-library
#
message(STATUS "Installing tiny-process-library via submodule")
execute_process(COMMAND git submodule update --init -- external/tiny-process-library
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Threads REQUIRED QUIET)

# create a target here instead of using the one provided
set(tiny_process_dir ${CMAKE_CURRENT_SOURCE_DIR}/external/tiny-process-library)
if(WIN32)
    add_library(_cppast_tiny_process EXCLUDE_FROM_ALL
                    ${tiny_process_dir}/process.hpp
                    ${tiny_process_dir}/process.cpp
                    ${tiny_process_dir}/process_win.cpp)
else()
    add_library(_cppast_tiny_process EXCLUDE_FROM_ALL
                    ${tiny_process_dir}/process.hpp
                    ${tiny_process_dir}/process.cpp
                    ${tiny_process_dir}/process_unix.cpp)
endif()
target_include_directories(_cppast_tiny_process PUBLIC ${tiny_process_dir})
target_link_libraries(_cppast_tiny_process PUBLIC Threads::Threads)
set_target_properties(_cppast_tiny_process PROPERTIES CXX_STANDARD 11)

#
# install cxxopts, if needed
#
if(build_tool)
    message(STATUS "Installing cxxopts via submodule")
    execute_process(COMMAND git submodule update --init -- external/cxxopts
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/cxxopts EXCLUDE_FROM_ALL)
endif()

#
# install libclang
#

# downloads and extracts LLVM using the given version and OS name
# sets: LLVM_DOWNLOAD_DIR
function(_cppast_download_llvm version os)
    set(folder "clang+llvm-${version}-${os}") # name of downloaded folder

    if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${folder})
        set(url http://releases.llvm.org/${version}/${folder}.tar.xz) # download URL
        message(STATUS "Downloading LLVM from ${url}")
        file(DOWNLOAD ${url} ${CMAKE_CURRENT_BINARY_DIR}/${folder}.tar.xz SHOW_PROGRESS
            STATUS status
            LOG log)

        list(GET status 0 status_code)
        list(GET status 1 status_string)
        if(NOT status_code EQUAL 0)
            message(FATAL_ERROR "error downloading llvm: ${status_string}" "${log}")
        endif()

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xJf ${folder}.tar.xz
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    set(LLVM_DOWNLOAD_DIR ${CMAKE_CURRENT_BINARY_DIR}/${folder} PARENT_SCOPE)
endfunction()

# finds the llvm-config binary
# sets: LLVM_CONFIG_BINARY
function(_cppast_find_llvm_config)
    unset(LLVM_CONFIG_BINARY CACHE)
    if (LLVM_DOWNLOAD_DIR)
        find_program(LLVM_CONFIG_BINARY "llvm-config" "${LLVM_DOWNLOAD_DIR}/bin" NO_DEFAULT_PATH)
    else()
        find_program(LLVM_CONFIG_BINARY "llvm-config")
    endif()
    if(NOT LLVM_CONFIG_BINARY)
        message(FATAL_ERROR "Unable to find llvm-config binary, please set option LLVM_CONFIG_BINARY yourself")
    else()
        message(STATUS "Found llvm-config at ${LLVM_CONFIG_BINARY}")
    endif()
endfunction()

# find libclang using the config tool
# sets: LLVM_VERSION, LIBCLANG_INCLUDE_DIR, LIBCLANG_SYSTEM_INCLUDE_DIR, LIBCLANG_LIBRARY and CLANG_BINARY
function(_cppast_find_libclang config_tool min_version force)
    if (NOT EXISTS "${LLVM_CONFIG_BINARY}")
        message(FATAL_ERROR "LLVM config binary not found at ${LLVM_CONFIG_BINARY}")
    endif()

    # check version
    execute_process(COMMAND ${LLVM_CONFIG_BINARY} --version
                    OUTPUT_VARIABLE llvm_version OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(llvm_version VERSION_LESS min_version)
        message(FATAL_ERROR "Outdated LLVM version ${llvm_version}, minimal supported is ${min_version}")
    else()
        message(STATUS "Using LLVM version ${llvm_version}")
        set(LLVM_VERSION ${llvm_version} CACHE INTERNAL "")
    endif()

    # get include directory
    if(${force})
        unset(LIBCLANG_INCLUDE_DIR CACHE)
    endif()
    if(NOT LIBCLANG_INCLUDE_DIR)
        execute_process(COMMAND ${config_tool} --includedir
                        OUTPUT_VARIABLE llvm_include_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        find_path(LIBCLANG_INCLUDE_DIR "clang-c/Index.h" "${llvm_include_dir}" NO_DEFAULT_PATH)

        if(NOT LIBCLANG_INCLUDE_DIR)
            message(FATAL_ERROR "libclang header files not found")
        else()
            message(STATUS "Found libclang header files at ${LIBCLANG_INCLUDE_DIR}")
        endif()
    endif()

    # find libclang library in llvm_library_dir
    if(${force})
        unset(LIBCLANG_LIBRARY CACHE)
    endif()
    if(NOT LIBCLANG_LIBRARY)
        execute_process(COMMAND ${config_tool} --libdir
                        OUTPUT_VARIABLE llvm_library_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        find_library(LIBCLANG_LIBRARY "clang" "${llvm_library_dir}" NO_DEFAULT_PATH)

        if(NOT LIBCLANG_LIBRARY)
            message(FATAL_ERROR "libclang library not found")
        else()
            message(STATUS "Found libclang library at ${LIBCLANG_LIBRARY}")
        endif()
    endif()

    # find system header files in llvm_library_dir
    if(${force})
        unset(LIBCLANG_SYSTEM_INCLUDE_DIR CACHE)
    endif()
    if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR)
        find_path(LIBCLANG_SYSTEM_INCLUDE_DIR "stddef.h" "${llvm_library_dir}/clang/${llvm_version}/include" NO_DEFAULT_PATH)

        if(NOT LIBCLANG_SYSTEM_INCLUDE_DIR)
            message(FATAL_ERROR "libclang system header files not found")
        else()
            message(STATUS "Found libclang system header files at ${LIBCLANG_SYSTEM_INCLUDE_DIR}")
        endif()
    endif()

    # find clang binary in llvm_binary_dir
    if(${force})
        unset(CLANG_BINARY CACHE)
    endif()
    if(NOT CLANG_BINARY)
        execute_process(COMMAND ${config_tool} --bindir
                        OUTPUT_VARIABLE llvm_binary_dir OUTPUT_STRIP_TRAILING_WHITESPACE)
        find_program(CLANG_BINARY "clang" "${llvm_binary_dir}" NO_DEFAULT_PATH)

        if(NOT CLANG_BINARY)
            message(FATAL_ERROR "clang binary not found")
        else()
            message(STATUS "Found clang binary at ${CLANG_BINARY}")
        endif()
    endif()
endfunction()

set(llvm_min_version 3.9.1)
set(LLVM_PREFERRED_VERSION 4.0.0 CACHE STRING "the preferred LLVM version")

if(DEFINED LLVM_VERSION_EXPLICIT)
    if(LLVM_VERSION_EXPLICIT VERSION_LESS llvm_min_version)
        message(FATAL_ERROR "Outdated LLVM version ${LLVM_VERSION_EXPLICIT}, minimal supported is ${llvm_min_version}")
    endif()
    set(LLVM_VERSION ${LLVM_VERSION_EXPLICIT} CACHE INTERNAL "")
    message(STATUS "Using manually specified LLVM version ${LLVM_VERSION}")
elseif(NOT LLVM_CONFIG_BINARY)
    if(DEFINED LLVM_DOWNLOAD_OS_NAME)
        _cppast_download_llvm(${LLVM_PREFERRED_VERSION} ${LLVM_DOWNLOAD_OS_NAME})
    endif()
    _cppast_find_llvm_config()
    _cppast_find_libclang(${LLVM_CONFIG_BINARY} ${llvm_min_version} 1) # override here
else()
    _cppast_find_libclang(${LLVM_CONFIG_BINARY} ${llvm_min_version} 0)
endif()

add_library(_cppast_libclang INTERFACE)
target_link_libraries(_cppast_libclang INTERFACE ${LIBCLANG_LIBRARY})
target_include_directories(_cppast_libclang INTERFACE ${LIBCLANG_INCLUDE_DIR})
target_compile_definitions(_cppast_libclang INTERFACE
                           CPPAST_LIBCLANG_SYSTEM_INCLUDE_DIR="${LIBCLANG_SYSTEM_INCLUDE_DIR}"
                           CPPAST_CLANG_BINARY="${CLANG_BINARY}"
                           CPPAST_CLANG_VERSION_STRING="${LLVM_VERSION}")
