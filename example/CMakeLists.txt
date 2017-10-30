# Copyright (C) 2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

function(_cppast_example name)
    add_executable(cppast_example_${name} ${name}.cpp)
    target_link_libraries(cppast_example_${name} PUBLIC cppast)
endfunction()

_cppast_example(ast_printer)
