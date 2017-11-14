# cppast - examples

This directory contains example tools written using cppast.

***Note:** These are not meant to be production ready tools, just a proof of concept!*

All example executables get a single parameter,
which is the directory where a `compile_commands.json` file is located.
This file is a compilation database, you can read more about it [here](https://clang.llvm.org/docs/JSONCompilationDatabase.html).
CMake, for example, can generate on when the option `CMAKE_EXPORT_COMPILE_COMMANDS` is `ON`.

The tools will parse each file in the database and process it.
Output will be written to `stdout`.

## List of examples:

### `ast_printer.cpp`

It is a very simplified implementation of the cppast tool, it will print an AST.
This is the starting example, it showcases entity visitation.

### `documentation_generator.cpp`

It is a very simplified documentation generator.
This showcases usage of the `cppast::code_generator`.

### Attributes example

* `comparison.cpp`
* `documentation_generator.cpp`
* `enum_category.cpp`
* `enum_to_string.cpp`
* `serialization.cpp`

Those examples were created as a part of my talk [Fun With (User-Defined) Attributes](http://foonathan.net/meetingcpp2017.html).
Check the talk video to learn more about them.
