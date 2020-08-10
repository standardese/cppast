# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

if(NOT CPPAST_TYPE_SAFE_URL)
    set(CPPAST_TYPE_SAFE_URL https://github.com/Manu343726/type_safe)
endif()
if(NOT CPPAST_TYPE_SAFE_VERSION)
    set(CPPAST_TYPE_SAFE_VERSION d35301f6c03b054fbad989f1490cb53c5cc782f2)
endif()
if(NOT CPPAST_TINY-PROCESS-LIBRARY_URL)
    set(CPPAST_TINY-PROCESS-LIBRARY_URL https://github.com/eidheim/tiny-process-library)
endif()
if(NOT CPPAST_TINY-PROCESS-LIBRARY_VERSION)
    set(CPPAST_TINY-PROCESS-LIBRARY_VERSION 5bc47531a97f80ac07092ac3ed56a26139c748b3)
endif()
if(NOT CPPAST_CXXOPTS_URL)
    set(CPPAST_CXXOPTS_URL https://github.com/jarro2783/cxxopts)
endif()
if(NOT CPPAST_CXXOPTS_VERSION)
    set(CPPAST_CXXOPTS_VERSION 3876c0907237e5fa89c5850ed1ee688a3bcb62b3)
endif()

include(FetchContent)

FetchContent_Declare(type_safe
    GIT_REPOSITORY "${CPPAST_TYPE_SAFE_URL}"
    GIT_TAG "${CPPAST_TYPE_SAFE_VERSION}"
)

FetchContent_Declare(tiny-process-library
    GIT_REPOSITORY "${CPPAST_TINY-PROCESS-LIBRARY_URL}"
    GIT_TAG "${CPPAST_TINY-PROCESS-LIBRARY_VERSION}"
)

FetchContent_Declare(cxxopts
    GIT_REPOSITORY "${CPPAST_CXXOPTS_URL}"
    GIT_TAG "${CPPAST_CXXOPTS_VERSION}"
)


#
# install type safe
#
find_package(type_safe QUIET)
if(NOT type_safe_FOUND)
    FetchContent_GetProperties(type_safe)

    if(NOT type_safe_POPULATED)
        FetchContent_Populate(type_safe)
        add_subdirectory("${type_safe_SOURCE_DIR}" "${type_safe_BINARY_DIR}" EXCLUDE_FROM_ALL)
    endif()
else()
    if((NOT TARGET type_safe) AND (TARGET type_safe::type_safe))
        add_library(type_safe INTERFACE)
        target_link_libraries(type_safe INTERFACE type_safe::type_safe)
    endif()
endif()

#
# install the tiny-process-library
#
find_package(Threads REQUIRED QUIET)

find_package(tiny_process QUIET)

if(tiny_process_FOUND)
    add_library(_cppast_tiny_process INTERFACE)
    target_link_libraries(_cppast_tiny_process INTERFACE tiny_process::tiny_process)
else()
    FetchContent_GetProperties(tiny-process-library)

    if(NOT tiny-process-library_POPULATED)
        FetchContent_Populate(tiny-process-library)
    endif()

    # create a target here instead of using the one provided

    if(WIN32)
        add_library(_cppast_tiny_process EXCLUDE_FROM_ALL
            ${tiny-process-library_SOURCE_DIR}/process.hpp
            ${tiny-process-library_SOURCE_DIR}/process.cpp
            ${tiny-process-library_SOURCE_DIR}/process_win.cpp)
    else()
        add_library(_cppast_tiny_process EXCLUDE_FROM_ALL
            ${tiny-process-library_SOURCE_DIR}/process.hpp
            ${tiny-process-library_SOURCE_DIR}/process.cpp
            ${tiny-process-library_SOURCE_DIR}/process_unix.cpp)
    endif()
    target_include_directories(_cppast_tiny_process PUBLIC ${tiny-process-library_SOURCE_DIR})
    target_link_libraries(_cppast_tiny_process PUBLIC Threads::Threads)
    set_target_properties(_cppast_tiny_process PROPERTIES CXX_STANDARD 11)

    if(CPPAST_USE_OLD_LIBSTDCPP_ABI)
        target_compile_definitions(_cppast_tiny_process PUBLIC _GLIBCXX_USE_CXX11_ABI=0)
    else()
        target_compile_definitions(_cppast_tiny_process PUBLIC _GLIBCXX_USE_CXX11_ABI=1)
    endif()
endif()

#
# install cxxopts, if needed
#
if(build_tool)
    find_package(cxxopts QUIET)

    if(cxxopts_FOUND)
        add_library(cxxopts INTERFACE)
        target_link_libraries(cxxopts INTERFACE cxxopts::cxxopts)
    else()
        FetchContent_GetProperties(cxxopts)

        if(NOT cxxopts_POPULATED)
            FetchContent_Populate(cxxopts)
            add_subdirectory("${cxxopts_SOURCE_DIR}" "${cxxopts_BINARY_DIR}" EXCLUDE_FROM_ALL)
        endif()
    endif()
endif()

#
# install libclang
#

function(name_without_extension FILENAME NAME)
    set(known_extensions "\\.tar\\.xz" "\\.tar\\.gz" "\\.tar" "\\.zip")

    set(name_we "${FILENAME}")

    foreach(ext ${known_extensions})
        string(REGEX REPLACE "(.*)${ext}" "\\1" name_we "${name_we}")
    endforeach()
    set(${NAME} "${name_we}" PARENT_SCOPE)
endfunction()

# downloads and extracts LLVM using the given URL, filename, and extension
# sets: LLVM_DOWNLOAD_DIR
function(_cppast_download_llvm url)
    get_filename_component(file "${url}" NAME)
    name_without_extension(${file} folder)
    string(REPLACE "%2B" "+" folder "${folder}")

    if(NOT CPPAST_PRECOMPILED_LLVM_ARCHIVE_DIR)
        set(CPPAST_PRECOMPILED_LLVM_ARCHIVE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(archive_fullpath "${CPPAST_PRECOMPILED_LLVM_ARCHIVE_DIR}/${file}")
    set(extracted_fullpath "${CMAKE_CURRENT_BINARY_DIR}/${folder}")

    if(NOT EXISTS ${extracted_fullpath})
        if(NOT EXISTS ${archive_fullpath})
            message(STATUS "Downloading LLVM from ${url}")
            file(DOWNLOAD ${url} ${CMAKE_CURRENT_BINARY_DIR}/${file}
                STATUS status
                LOG log)

            list(GET status 0 status_code)
            list(GET status 1 status_string)
            if(NOT status_code EQUAL 0)
                message(FATAL_ERROR "error downloading llvm: ${status_string}" "${log}")
            endif()
        endif()

        message(STATUS "Extracting LLVM from ${archive_fullpath} to ${extracted_fullpath}")

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xJf ${archive_fullpath}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    set(LLVM_DOWNLOAD_DIR "${extracted_fullpath}" PARENT_SCOPE)
endfunction()

# downloads and extracts LLVM using the given version and OS name
# sets: LLVM_DOWNLOAD_DIR
function(_cppast_download_llvm_from_llvm_releases version os)
    _cppast_download_llvm("http://releases.llvm.org/${version}/clang+llvm-${version}-${os}.tar.xz")
    set(LLVM_DOWNLOAD_DIR ${LLVM_DOWNLOAD_DIR} PARENT_SCOPE)
endfunction()

# determines the llvm version from a config binary
macro(_cppast_llvm_version output_name llvm_binary)
    execute_process(COMMAND ${llvm_binary} --version
                    OUTPUT_VARIABLE ${output_name} OUTPUT_STRIP_TRAILING_WHITESPACE)

    # ignore git tags in the version string, get the semver number only
    string(REGEX REPLACE "([0-9]).([0-9]).([0-9])(.*)" "\\1.\\2.\\3" ${output_name} "${${output_name}}")
endmacro()

# finds the llvm-config binary
# sets: LLVM_CONFIG_BINARY
function(_cppast_find_llvm_config)
    unset(LLVM_CONFIG_BINARY CACHE)
    if(LLVM_DOWNLOAD_DIR)
        find_program(LLVM_CONFIG_BINARY "llvm-config" "${LLVM_DOWNLOAD_DIR}/bin" NO_DEFAULT_PATH)
    else()
        find_program(llvm_config_binary_no_suffix llvm-config)
        find_program(llvm_config_binary_suffix NAMES llvm-config-7 llvm-config-6.0 llvm-config-5.0 llvm-config-4.0)

        if(NOT llvm_config_binary_no_suffix)
            set(LLVM_CONFIG_BINARY ${llvm_config_binary_suffix} CACHE INTERNAL "")
        elseif(NOT llvm_config_binary_suffix)
            set(LLVM_CONFIG_BINARY ${llvm_config_binary_no_suffix} CACHE INTERNAL "")
        else()
            # pick latest version of the two
            _cppast_llvm_version(suffix_version ${llvm_config_binary_suffix})
            _cppast_llvm_version(no_suffix_version ${llvm_config_binary_no_suffix})
            if(suffix_version VERSION_GREATER no_suffix_version)
                set(LLVM_CONFIG_BINARY ${llvm_config_binary_suffix} CACHE INTERNAL "")
            else()
                set(LLVM_CONFIG_BINARY ${llvm_config_binary_no_suffix} CACHE INTERNAL "")
            endif()
        endif()
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

    _cppast_llvm_version(llvm_version ${config_tool})
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
        endif()
        message(STATUS "Found libclang library at ${LIBCLANG_LIBRARY}")

        get_filename_component(ext ${LIBCLANG_LIBRARY} EXT)
        if(UNIX AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND "${ext}" STREQUAL ".a")
            message(STATUS "libclang will be linked statically; linking might take a long time")

            # glob all libraries and put them inside a group,
            # as the correct order cannot be determined apparently
            file(GLOB clang_libraries "${llvm_library_dir}/libclang*.a")
            string(REPLACE ";" " " clang_libraries "${clang_libraries}")
            set(clang_libraries "-Wl,--start-group ${clang_libraries} -Wl,--end-group")

            file(GLOB llvm_libraries "${llvm_library_dir}/libLLVM*.a")
            string(REPLACE ";" " " llvm_libraries "${llvm_libraries}")
            set(llvm_libraries "-Wl,--start-group ${llvm_libraries} -Wl,--end-group")

            set(LIBCLANG_LIBRARY "${clang_libraries} ${llvm_libraries}" CACHE INTERNAL "")
        endif()
    endif()

    # find clang binary in llvm_binary_dir
    # note: never override that binary
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

set(llvm_min_version 4.0.0)

if(NOT DEFINED LLVM_PREFERRED_VERSION)
    set(LLVM_PREFERRED_VERSION 4.0.0 CACHE STRING "the preferred LLVM version")
endif()

find_package(libclang QUIET)
add_library(_cppast_libclang INTERFACE)

if(libclang_FOUND)
    target_link_libraries(_cppast_libclang INTERFACE libclang::libclang)

    if(NOT CLANG_BINARY)
        set(libclang_ROOT_DIR "${libclang_LIB_DIRS}/..")
        set(libclang_BINARY_DIR "${libclang_ROOT_DIR}/bin")

        find_program(CLANG_BINARY "clang" "${libclang_BINARY_DIR}" NO_DEFAULT_PATH)

        if(NOT CLANG_BINARY)
            message(FATAL_ERROR "clang binary not found (Search started from ${libclang_BINARY_DIR})")
        else()
            message(STATUS "Found clang binary at ${CLANG_BINARY}")
        endif()
    endif()

    set(LLVM_VERSION "${libclang_VERSION}")
else()
    if(DEFINED LLVM_VERSION_EXPLICIT)
        if(LLVM_VERSION_EXPLICIT VERSION_LESS llvm_min_version)
            message(FATAL_ERROR "Outdated LLVM version ${LLVM_VERSION_EXPLICIT}, minimal supported is ${llvm_min_version}")
        endif()
        set(LLVM_VERSION ${LLVM_VERSION_EXPLICIT} CACHE INTERNAL "")
        message(STATUS "Using manually specified LLVM version ${LLVM_VERSION}")
    elseif(NOT LLVM_CONFIG_BINARY)
        if(DEFINED LLVM_DOWNLOAD_OS_NAME)
            _cppast_download_llvm_from_llvm_releases(${LLVM_PREFERRED_VERSION} ${LLVM_DOWNLOAD_OS_NAME})
        elseif(DEFINED LLVM_DOWNLOAD_URL)
            _cppast_download_llvm(${LLVM_DOWNLOAD_URL})
        endif()
        _cppast_find_llvm_config()
        _cppast_find_libclang(${LLVM_CONFIG_BINARY} ${llvm_min_version} 1) # override here
    else()
        _cppast_find_libclang(${LLVM_CONFIG_BINARY} ${llvm_min_version} 0)
    endif()
endif()

target_link_libraries(_cppast_libclang INTERFACE ${LIBCLANG_LIBRARY})
target_include_directories(_cppast_libclang INTERFACE ${LIBCLANG_INCLUDE_DIR})
target_compile_definitions(_cppast_libclang INTERFACE
                           CPPAST_CLANG_BINARY="${CLANG_BINARY}"
                           CPPAST_CLANG_VERSION_STRING="${LLVM_VERSION}")
