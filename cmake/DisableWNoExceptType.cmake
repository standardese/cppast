# This script tries to detectif the warning -Wnoexcept-type is available in
# the active C++ compiler and tries to disable it, since it causes warnings about
# C++17 mangling changes in type_safe code. Of course this is just a workaround until
# we could check what exactly is going on in type_safe

function(disable_wnoexcept_type_on_target TARGET)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -Q --help=warning
            COMMAND grep noexcept-type
            RESULT_VARIABLE result
            OUTPUT_VARIABLE str
        )

        if(result EQUAL 0)
            message(STATUS "GCC supports -Wnoexcept-type warning, disabled")
            target_compile_options(${TARGET} PUBLIC -Wno-noexcept-type)
        endif()
    endif()
endfunction()
