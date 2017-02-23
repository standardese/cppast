cmake_minimum_required(VERSION 3.6)
project(cppast)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES include/cppast/cpp_entity.hpp include/cppast/cpp_entity_kind.hpp include/cppast/detail/intrusive_list.hpp include/cppast/detail/assert.hpp include/cppast/cpp_entity_container.hpp src/cpp_entity_kind.cpp include/cppast/cpp_file.hpp src/cpp_file.cpp include/cppast/cpp_namespace.hpp include/cppast/cpp_entity_index.hpp include/cppast/cpp_entity_ref.hpp include/cppast/cpp_type.hpp src/cpp_namespace.cpp src/cpp_type.cpp include/cppast/cpp_expression.hpp include/cppast/cpp_array_type.hpp include/cppast/cpp_function_type.hpp include/cppast/cpp_enum.hpp src/cpp_enum.cpp src/cpp_entity.cpp include/cppast/visitor.hpp src/visitor.cpp src/cpp_type_alias.cpp include/cppast/cpp_type_alias.hpp include/cppast/cpp_variable_base.hpp src/cpp_variable.cpp include/cppast/cpp_variable.hpp src/cpp_language_linkage.cpp include/cppast/cpp_language_linkage.hpp src/cpp_function.cpp include/cppast/cpp_function.hpp include/cppast/cpp_storage_specifiers.hpp src/cpp_class.cpp include/cppast/cpp_class.hpp src/cpp_member_variable.cpp include/cppast/cpp_member_variable.hpp src/cpp_member_function.cpp include/cppast/cpp_member_function.hpp src/cpp_template_parameter.cpp include/cppast/cpp_template_parameter.hpp include/cppast/cpp_alias_template.hpp src/cpp_alias_template.cpp include/cppast/cpp_template.hpp src/cpp_function_template.cpp include/cppast/cpp_function_template.hpp src/cpp_class_template.cpp include/cppast/cpp_class_template.hpp src/cpp_variable_template.cpp include/cppast/cpp_variable_template.hpp)
add_library(cppast ${SOURCE_FILES})
target_include_directories(cppast PUBLIC ../type_safe/include ../debug_assert include/)

add_executable(cppast_main main.cpp)
target_link_libraries(cppast_main PUBLIC cppast)