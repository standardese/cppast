# Copyright (C) 2017-2023 Jonathan Müller and cppast contributors
# SPDX-License-Identifier: MIT
# found in the top-level directory of this distribution.

add_executable(cppast_tool main.cpp)
target_link_libraries(cppast_tool PUBLIC cppast cxxopts)
set_target_properties(cppast_tool PROPERTIES CXX_STANDARD 11 OUTPUT_NAME cppast)

install(TARGETS cppast_tool)
